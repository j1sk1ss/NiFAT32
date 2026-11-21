/* Spatial address-range model. Coordinates are assumptions unless supplied by
   the user; address extents always come from the formatter's existing model. */
(() => {
  'use strict';
  const canvas = document.getElementById('spatialcv');
  if (!canvas) return;
  const el = id => document.getElementById(id);
  const names = {fat:'FAT', bs:'Boot', journal:'Journal', error:'Errors'};
  const colors = {fat:'#ed6942', bs:'#a4c69c', journal:'#8ea9b7', error:'#c19cb9'};
  const model = {chip:null, custom:null, map:null, cells:[], copies:[], selected:null, exposed:new Set(), request:null,
    beam:{x:.5,y:.5,radius:.08,angle:30,shape:'spot',enabled:true}, box:null};
  const overlap = (a,b,c,d) => a < d && c < b;

  function gridMap() {
    // The finest common erase-size subdivision, never smaller than a NiFAT
    // sector. STM32's unequal erase sectors remain unequal groups of tiles.
    const flash = state.chip.flash;
    const unit = Math.max(512, Math.min(...(flash.sectors || [flash.erase])));
    const count = Math.ceil(flash.size / unit);
    const columns = Math.ceil(Math.sqrt(count));
    const rows = Math.ceil(count / columns);
    const snake = el('spatial-layout').value === 'snake';
    const regions = Array.from({length:count}, (_,i) => {
      const row = Math.floor(i/columns);
      const col = snake && row%2 ? columns-1-i%columns : i%columns;
      return {start:i*unit,size:Math.min(unit,flash.size-i*unit),x:col/columns,y:row/rows,width:1/columns,height:1/rows};
    });
    return {version:1,chipId:state.chip.id,flashSize:flash.size,unit:'normalized',width:1,height:1,
      source:`Illustrative ${snake ? 'serpentine' : 'row-major'} address grid; not measured die coordinates`,
      model:true,regions};
  }

  function validateMap(data) {
    if (!data || data.version !== 1 || data.chipId !== state.chip.id || data.flashSize !== state.chip.flash.size)
      throw Error('Map version, chipId, and flashSize must match the selected chip.');
    if (!Number.isFinite(data.width) || !Number.isFinite(data.height) || data.width<=0 || data.height<=0)
      throw Error('Array width and height must be positive numbers.');
    if (!['normalized','um','mm'].includes(data.unit)) throw Error('Coordinate unit must be normalized, um, or mm.');
    if (typeof data.source !== 'string' || !data.source.trim() || data.source.length>500)
      throw Error('Provide a source description of up to 500 characters.');
    if (!Array.isArray(data.regions) || !data.regions.length || data.regions.length>8192)
      throw Error('Provide between 1 and 8192 mapped address ranges.');
    const regions = data.regions.map(region => {
      const copy = {};
      for (const key of ['start','size','x','y','width','height']) {
        if (!Number.isFinite(region[key])) throw Error(`Region ${key} must be a finite number.`);
        copy[key]=region[key];
      }
      if (!Number.isSafeInteger(copy.start) || !Number.isSafeInteger(copy.size) || copy.start<0 || copy.size<=0)
        throw Error('Address offsets and sizes must be nonnegative/positive integer bytes.');
      if (copy.x<0 || copy.y<0 || copy.width<=0 || copy.height<=0 ||
          copy.x+copy.width>data.width+1e-10 || copy.y+copy.height>data.height+1e-10)
        throw Error('Every rectangle must fit inside the declared memory array.');
      return copy;
    }).sort((a,b)=>a.start-b.start);
    let end=0;
    for (const region of regions) {
      if (region.start!==end) throw Error('Address ranges must cover the flash exactly, with no gaps or overlaps.');
      end=region.start+region.size;
    }
    if (end!==data.flashSize) throw Error('Address ranges must end at flashSize.');
    // A single-plane map cannot assign the same physical area to two ranges.
    // Sweep along X to avoid quadratic work for a typical fine-grained grid.
    const sorted=[...regions].sort((a,b)=>a.x-b.x);
    for (let i=0;i<sorted.length;i++) {
      const a=sorted[i];
      for(let j=i+1;j<sorted.length && sorted[j].x<a.x+a.width-1e-10;j++) {
        const b=sorted[j];
        if (a.y < b.y+b.height-1e-10 && b.y < a.y+a.height-1e-10)
          throw Error('Physical rectangles overlap. Supply a single-plane map with separate rectangles.');
      }
    }
    return {version:1,chipId:data.chipId,flashSize:data.flashSize,source:data.source,unit:data.unit,
      width:data.width,height:data.height,model:data.model===true,regions};
  }

  function bindRanges() {
    const map=model.map;
    const scale=Math.max(map.width,map.height);
    const ox=(1-map.width/scale)/2, oy=(1-map.height/scale)/2;
    const volumeEnd=state.offset+G.ts*512;
    model.copies=G.structs.map(st => {
      const start=state.offset+st.sec*512, end=start+st.size*512;
      return {...st,key:`${st.kind}-${st.i}`,name:`${names[st.kind]} ${st.i+1}`,start,end,
        invalid:start<state.offset || end>volumeEnd || end>state.chip.flash.size,cells:[]};
    });
    const bounds=eraseRanges();
    let eraseIndex=0;
    model.cells=map.regions.map((region,index) => {
      while(eraseIndex+1<bounds.length && bounds[eraseIndex].end<=region.start)eraseIndex++;
      const end=region.start+region.size;
      let eraseEndIndex=eraseIndex;
      while(eraseEndIndex+1<bounds.length && bounds[eraseEndIndex].end<end)eraseEndIndex++;
      const cell={...region,index,x:ox+region.x/scale,y:oy+region.y/scale,
        width:region.width/scale,height:region.height/scale,copies:[],eraseIndex,eraseEndIndex,
        inVolume:overlap(region.start,end,state.offset,Math.min(volumeEnd,state.chip.flash.size))};
      for (const copy of model.copies) {
        if (overlap(region.start,end,copy.start,Math.min(copy.end,volumeEnd,state.chip.flash.size))) {
          cell.copies.push(copy);copy.cells.push(index);
        }
      }
      return cell;
    });
    if (!model.copies.some(copy=>copy.key===model.selected)) model.selected=null;
    let nearest=null;
    const fats=model.copies.filter(copy=>copy.kind==='fat'&&copy.cells.length);
    for(let i=0;i<fats.length;i++)for(let j=i+1;j<fats.length;j++) {
      for(const ai of fats[i].cells)for(const bi of fats[j].cells) {
        const a=model.cells[ai],b=model.cells[bi];
        const dx=Math.max(0,a.x-b.x-b.width,b.x-a.x-a.width);
        const dy=Math.max(0,a.y-b.y-b.height,b.y-a.y-a.height);
        const distance=Math.hypot(dx,dy);
        if(!nearest||distance<nearest.distance)nearest={distance,a:fats[i].name,b:fats[j].name};
      }
    }
    const distanceUnit=map.unit==='normalized' ? '% of array width' : map.unit==='um' ? 'µm' : 'mm';
    const value=nearest ? (map.unit==='normalized' ? nearest.distance*scale/map.width*100 : nearest.distance*scale) : 0;
    el('spatial-separation').textContent=nearest ? `Closest FAT copy tiles: ${nearest.a} / ${nearest.b} · ${Number(value.toPrecision(3))} ${distanceUnit}. Distance is between tile edges, at this map’s resolution.` : 'Select at least two FAT copies to compare their mapped separation.';
  }

  function eraseRanges() {
    const flash=state.chip.flash;
    let start=0;
    return (flash.sectors || Array.from({length:Math.ceil(flash.size/flash.erase)},()=>flash.erase))
      .map(size=>{const range={start,end:Math.min(flash.size,start+size)};start+=size;return range;});
  }

  function intersects(cell, beam=model.beam) {
    if (!beam.enabled) return false;
    if(beam.shape==='track') {
      const theta=beam.angle*Math.PI/180, nx=-Math.sin(theta), ny=Math.cos(theta);
      const distance=Math.abs((cell.x+cell.width/2-beam.x)*nx+(cell.y+cell.height/2-beam.y)*ny);
      return distance<=beam.radius+(Math.abs(nx)*cell.width+Math.abs(ny)*cell.height)/2+1e-12;
    }
    const x=Math.max(cell.x,Math.min(beam.x,cell.x+cell.width));
    const y=Math.max(cell.y,Math.min(beam.y,cell.y+cell.height));
    return (x-beam.x)**2+(y-beam.y)**2<=beam.radius**2+1e-12;
  }

  function update() {
    if (!G) return;
    if (model.chip!==state.chip.id) {
      model.chip=state.chip.id;model.custom=null;model.selected=null;
      el('physical-map-status').textContent='';
      el('physical-map-file').value='';
    }
    model.map=model.custom || gridMap();
    model.beam.radius=Number(el('beam-radius').value)/100*model.map.width/Math.max(model.map.width,model.map.height);
    bindRanges();
    el('spatial-layout').disabled=!!model.custom;
    el('reset-physical-map').hidden=!model.custom;
    const imported=!!model.custom;
    el('mapping-badge').textContent=imported && !model.map.model ? 'SUPPLIED COORDINATES' : 'GRID MODEL';
    const flashType=state.chip.id.startsWith('esp') || state.chip.id==='rp2040' ? 'External flash memory' : 'Internal program flash';
    const sizes=[...new Set(model.map.regions.map(r=>r.size))];
    el('mapping-description').textContent=`${flashType} · ${model.map.regions.length} mapped tiles${sizes.length===1 ? ` · ${fmtB(sizes[0])} per tile` : ' · variable tile sizes'}. `+
      (imported && !model.map.model ? `Coordinates supplied by user: ${model.map.source}. ` : 'Grid positions are an address-to-space assumption, not a measured chip floorplan. ')+
      'Offsets and copy extents use the current NiFAT32 geometry. A beam touching a tile exposes its entire address range.';
    el('spatial-legend').innerHTML=Object.entries(names).map(([kind,name])=>`<span><i class="dot" style="background:${colors[kind]}"></i>${name}</span>`).join('')+
      '<span><i class="dot" style="background:#45392b"></i>Volume data</span><span><i class="dot beam-dot"></i>Beam / exposed tile</span>';
    render();
  }

  function render() {
    if (!model.map) return;
    const {ctx,w}=setupCanvas(canvas,canvas.clientWidth || 600);
    const pad=w*.16, side=w*.68;
    model.box={x:pad,y:pad,size:side,w};
    ctx.clearRect(0,0,w,w);
    ctx.fillStyle='#100e0d';ctx.fillRect(0,0,w,w);
    // Square schematic package: the array is inset and has its own scale.
    ctx.fillStyle='#796044';
    for(let i=0;i<12;i++) {
      const position=w*(.19+i*.056);
      ctx.fillRect(position,w*.035,w*.018,w*.05);ctx.fillRect(position,w*.915,w*.018,w*.05);
      ctx.fillRect(w*.035,position,w*.05,w*.018);ctx.fillRect(w*.915,position,w*.05,w*.018);
    }
    ctx.fillStyle='#1b1511';ctx.strokeStyle='#98754d';ctx.lineWidth=1.5;
    ctx.beginPath();ctx.roundRect(w*.08,w*.08,w*.84,w*.84,w*.025);ctx.fill();ctx.stroke();
    ctx.fillStyle='#d9b88e';ctx.font=`${Math.max(9,w*.018)}px ui-monospace,monospace`;ctx.textAlign='left';
    ctx.fillText(state.chip.name.toUpperCase(),pad,w*.125,side*.8);
    ctx.fillStyle='#977c60';ctx.textAlign='right';ctx.fillText('FLASH',w-pad,w*.125);
    ctx.fillStyle='#0d0b09';ctx.fillRect(pad,pad,side,side);
    model.exposed=new Set();
    const filter=el('spatial-filter').value;
    const selected=model.copies.find(copy=>copy.key===model.selected);
    let exposedBytes=0;
    for(const cell of model.cells) {
      cell.hit=intersects(cell);
      if(cell.hit) {
        exposedBytes+=cell.size;
        cell.copies.forEach(copy=>model.exposed.add(copy.key));
      }
      const visible=cell.copies.filter(copy=>filter==='all' || copy.kind===filter);
      let color=cell.inVolume ? '#45392b' : '#29231e';
      const region=state.chip.regions.find(region=>overlap(cell.start,cell.start+cell.size,region.off,region.off+region.size));
      if(!cell.inVolume && region)color=region.color;
      if(visible.length)color=colors[visible[0].kind];
      const x=pad+cell.x*side,y=pad+cell.y*side,cw=cell.width*side,ch=cell.height*side;
      ctx.globalAlpha=selected && !cell.copies.includes(selected) ? .3 : 1;
      ctx.fillStyle=color;ctx.fillRect(x+.4,y+.4,Math.max(.6,cw-.8),Math.max(.6,ch-.8));
      if(cell.hit) {
        ctx.fillStyle='#ff454d4d';ctx.fillRect(x,y,cw,ch);
        ctx.strokeStyle='#ff7b81';ctx.lineWidth=1;ctx.strokeRect(x+.7,y+.7,Math.max(0,cw-1.4),Math.max(0,ch-1.4));
      }
      if(selected && cell.copies.includes(selected)) {
        ctx.strokeStyle='#fff0d3';ctx.lineWidth=2;ctx.strokeRect(x+.6,y+.6,cw-1.2,ch-1.2);
      }
    }
    ctx.globalAlpha=1;
    // Outline real erase-unit groupings without claiming their physical shape.
    if(!model.custom && state.chip.flash.sectors) {
      ctx.strokeStyle='#e2c09266';ctx.lineWidth=1.2;
      const ranges=eraseRanges();
      for(const range of ranges) {
        const cells=model.cells.filter(c=>overlap(c.start,c.start+c.size,range.start,range.end));
        // Draw the boundary of the tile union by omitting shared edges.
        for(const cell of cells) {
          const left=pad+cell.x*side,top=pad+cell.y*side,right=left+cell.width*side,bottom=top+cell.height*side;
          const near=(a,b)=>Math.abs(a-b)<1e-9;
          const shared=(edge)=>cells.some(c=>c!==cell && (edge==='left' ? near(c.x+c.width,cell.x)&&near(c.y,cell.y) : edge==='right' ? near(cell.x+cell.width,c.x)&&near(c.y,cell.y) : edge==='top' ? near(c.y+c.height,cell.y)&&near(c.x,cell.x) : near(cell.y+cell.height,c.y)&&near(c.x,cell.x)));
          ctx.beginPath();
          if(!shared('left')){ctx.moveTo(left,top);ctx.lineTo(left,bottom);}
          if(!shared('right')){ctx.moveTo(right,top);ctx.lineTo(right,bottom);}
          if(!shared('top')){ctx.moveTo(left,top);ctx.lineTo(right,top);}
          if(!shared('bottom')){ctx.moveTo(left,bottom);ctx.lineTo(right,bottom);}
          ctx.stroke();
        }
      }
    }
    // Copy labels sit on one of their actual intersecting tiles, never on a
    // bounding-box centroid that could lie in an unrelated address range.
    const labelPositions=new Map();
    const copies=[...model.copies].sort((a,b)=>(a.key===model.selected ? 1 : 0)-(b.key===model.selected ? 1 : 0));
    for(const copy of copies) {
      if(!copy.cells.length || (filter!=='all' && copy.kind!==filter))continue;
      if(selected && copy!==selected)continue;
      const cell=model.cells[copy.cells[Math.floor(copy.cells.length/2)]];
      const key=cell.index, n=labelPositions.get(key)||0;labelPositions.set(key,n+1);
      const x=pad+(cell.x+cell.width/2)*side,y=pad+(cell.y+cell.height/2)*side;
      const label=`${{fat:'F',bs:'B',journal:'J',error:'E'}[copy.kind]}${copy.i+1}`;
      const fs=Math.max(9,Math.min(11,w*.021));ctx.font=`600 ${fs}px ui-monospace,monospace`;
      const width=ctx.measureText(label).width+8;
      const lx=Math.max(pad,Math.min(pad+side-width,x-width/2)),ly=Math.max(pad,Math.min(pad+side-fs-5,y-fs/2+n*(fs+5)));
      ctx.strokeStyle=colors[copy.kind];ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(x,y);ctx.lineTo(lx+width/2,ly+fs/2);ctx.stroke();
      ctx.fillStyle='#100c09ed';ctx.fillRect(lx,ly,width,fs+5);ctx.strokeStyle=colors[copy.kind];ctx.lineWidth=1;ctx.strokeRect(lx,ly,width,fs+5);
      ctx.fillStyle=colors[copy.kind];ctx.textAlign='left';ctx.fillText(label,lx+4,ly+fs+1);
    }
    if(model.beam.enabled) {
      const beam=model.beam,cx=pad+beam.x*side,cy=pad+beam.y*side;
      ctx.save();ctx.beginPath();ctx.rect(pad,pad,side,side);ctx.clip();
      ctx.strokeStyle='#ff9a9f';ctx.fillStyle='#ff454d22';ctx.lineWidth=1.5;
      if(beam.shape==='spot') {ctx.beginPath();ctx.arc(cx,cy,beam.radius*side,0,Math.PI*2);ctx.fill();ctx.stroke();}
      else {
        ctx.translate(cx,cy);ctx.rotate(beam.angle*Math.PI/180);
        ctx.fillRect(-side*2,-beam.radius*side,side*4,2*beam.radius*side);
        ctx.strokeRect(-side*2,-beam.radius*side,side*4,2*beam.radius*side);
        ctx.rotate(-beam.angle*Math.PI/180);ctx.translate(-cx,-cy);
      }
      ctx.setLineDash([3,4]);ctx.beginPath();ctx.moveTo(cx-10,cy);ctx.lineTo(cx+10,cy);ctx.moveTo(cx,cy-10);ctx.lineTo(cx,cy+10);ctx.stroke();ctx.restore();
    }
    ctx.strokeStyle='#8b6c4b';ctx.lineWidth=1;ctx.strokeRect(pad,pad,side,side);
    ctx.fillStyle='#b69a7a';ctx.font=`${Math.max(8,w*.016)}px ui-monospace,monospace`;ctx.textAlign='left';
    ctx.fillText('0x00000000',pad,w*.88);ctx.textAlign='right';ctx.fillText(hex(state.chip.flash.size-1),pad+side,w*.88);
    renderControls(exposedBytes);
  }

  function renderControls(bytes) {
    const beam=model.beam,map=model.map;
    const scale=Math.max(map.width,map.height);
    const suffix=map.unit==='normalized' ? '% of array width' : map.unit==='um' ? 'µm' : 'mm';
    const radius=map.unit==='normalized' ? Math.round(beam.radius*100*scale/map.width) : Number((beam.radius*scale).toPrecision(4));
    el('beam-radius-name').textContent=beam.shape==='spot' ? 'Radius' : 'Track half-width';
    el('beam-radius-label').textContent=`${radius} ${suffix}`;
    el('beam-x-label').textContent=`${Math.round(beam.x*100)}%`;
    el('beam-y-label').textContent=`${Math.round(beam.y*100)}%`;
    el('beam-angle-label').textContent=`${beam.angle}°`;
    el('beam-angle-control').hidden=beam.shape!=='track';
    el('beam-toggle').textContent=beam.enabled ? 'Hide beam' : 'Show beam';
    el('beam-toggle').setAttribute('aria-pressed',String(beam.enabled));
    const lines=Object.keys(names).map(kind=>{
      const copies=model.copies.filter(copy=>copy.kind===kind);
      if(!copies.length)return '';
      const hit=copies.filter(copy=>model.exposed.has(copy.key)).length;
      const invalid=copies.filter(copy=>copy.invalid).length;
      const untouched=copies.filter(copy=>!model.exposed.has(copy.key)&&!copy.invalid).length;
      const label=kind==='fat' || kind==='journal' ? (untouched>copies.length/2 ? 'Untouched majority remains' : 'No untouched majority guaranteed') : (untouched ? `${untouched} untouched` : 'No untouched copy');
      return `<div class="beam-stat"><span>${names[kind]}</span><b class="${hit===copies.length || invalid ? 'bad' : hit ? 'warn' : 'ok'}">${hit} / ${copies.length} intersected</b><small>${label}${invalid ? ` · ${invalid} outside volume` : ''}</small></div>`;
    }).join('');
    const invalid=model.copies.filter(copy=>copy.invalid).length;
    const summary=`${model.exposed.size} / ${model.copies.length} copies intersected · ${fmtB(bytes)} in exposed tiles${invalid ? ` · ${invalid} copies outside volume` : ''}`;
    el('beam-summary').innerHTML=`<p class="beam-total">${summary}</p>${lines}`;
    const filter=el('spatial-filter').value;
    const signature=model.copies.filter(copy=>filter==='all'||copy.kind===filter).map(copy=>`${copy.key}:${model.exposed.has(copy.key)}:${copy.invalid}:${copy.key===model.selected}`).join('|');
    if(el('spatial-copies').dataset.signature!==signature) {
      const activeKey=document.activeElement?.dataset.copyKey;
      el('spatial-copies').dataset.signature=signature;
      el('spatial-copies').innerHTML=model.copies.filter(copy=>filter==='all'||copy.kind===filter).map(copy=>
        `<button type="button" data-copy-key="${copy.key}" class="spatial-copy${model.exposed.has(copy.key)?' exposed':''}" aria-pressed="${model.selected===copy.key}" style="--copy-color:${colors[copy.kind]}"><span>${copy.name}</span><small>${copy.invalid ? 'OUTSIDE VOLUME' : model.exposed.has(copy.key) ? 'INTERSECTED' : 'UNTOUCHED'}</small></button>`).join('');
      if(activeKey)el('spatial-copies').querySelector(`[data-copy-key="${activeKey}"]`)?.focus({preventScroll:true});
    }
    canvas.setAttribute('aria-label',`Square flash map for ${state.chip.name}. ${summary}. ${map.model ? 'Illustrative grid model.' : 'User-supplied coordinates.'} Arrow keys move the beam.`);
  }

  function schedule() {
    if(model.request!==null)return;
    model.request=requestAnimationFrame(()=>{model.request=null;render();});
  }
  ['beam-radius','beam-x','beam-y','beam-angle'].forEach(id=>{
    el(id).addEventListener('input',event=>{
      const key={'beam-radius':'radius','beam-x':'x','beam-y':'y','beam-angle':'angle'}[id];
      const value=Number(event.target.value);
      // Radius is always % of the array WIDTH, even for a rectangular imported die.
      model.beam[key]=key==='angle' ? value : key==='radius' ? value/100*model.map.width/Math.max(model.map.width,model.map.height) : value/100;
      schedule();
    });
  });
  el('beam-shape').addEventListener('change',event=>{model.beam.shape=event.target.value;schedule();});
  el('beam-toggle').addEventListener('click',()=>{model.beam.enabled=!model.beam.enabled;schedule();});
  el('spatial-layout').addEventListener('change',update);
  el('spatial-filter').addEventListener('change',()=>{model.selected=null;schedule();});
  el('spatial-copies').addEventListener('click',event=>{
    const button=event.target.closest('[data-copy-key]');if(!button)return;
    model.selected=model.selected===button.dataset.copyKey ? null : button.dataset.copyKey;
    // Keep the clicked button in the DOM so keyboard focus survives its update.
    schedule();
  });
  el('beam-target').addEventListener('click',()=>{
    const copy=model.copies.find(copy=>copy.kind==='fat'&&copy.cells.length);
    if(!copy)return;
    const cell=model.cells[copy.cells[0]];
    model.beam.x=cell.x+cell.width/2;model.beam.y=cell.y+cell.height/2;model.beam.enabled=true;
    el('beam-x').value=model.beam.x*100;el('beam-y').value=model.beam.y*100;
    schedule();
  });
  function point(event,move) {
    if(!model.box)return;
    const rect=canvas.getBoundingClientRect(),box=model.box;
    const x=(event.clientX-rect.left)*box.w/rect.width,y=(event.clientY-rect.top)*box.w/rect.height;
    const mx=(x-box.x)/box.size,my=(y-box.y)/box.size;
    const cell=model.cells.find(c=>mx>=c.x&&mx<=c.x+c.width&&my>=c.y&&my<=c.y+c.height);
    if(cell) {
      const copies=cell.copies.map(copy=>copy.name).join(', ')||'No metadata copy';
      const range=eraseRanges()[cell.eraseIndex];
      const units=cell.eraseEndIndex===cell.eraseIndex ? `erase unit ${cell.eraseIndex+1}` : `erase units ${cell.eraseIndex+1}–${cell.eraseEndIndex+1}`;
      el('spatial-inspect').textContent=`Flash offsets ${hex(cell.start)}–${hex(cell.start+cell.size-1)} · ${fmtB(cell.size)} · ${units} (${fmtB(range.end-range.start)} at range start) · ${copies}`;
    }
    if(move) {
      model.beam.x=Math.max(0,Math.min(1,mx));model.beam.y=Math.max(0,Math.min(1,my));model.beam.enabled=true;
      el('beam-x').value=model.beam.x*100;el('beam-y').value=model.beam.y*100;schedule();
    }
  }
  let dragging=false;
  canvas.addEventListener('pointerdown',event=>{dragging=true;canvas.setPointerCapture(event.pointerId);point(event,true);canvas.focus({preventScroll:true});});
  canvas.addEventListener('pointermove',event=>point(event,dragging));
  canvas.addEventListener('pointerup',()=>{dragging=false;});
  canvas.addEventListener('pointercancel',()=>{dragging=false;});
  canvas.addEventListener('lostpointercapture',()=>{dragging=false;});
  canvas.addEventListener('keydown',event=>{
    const shifts={ArrowLeft:[-1,0],ArrowRight:[1,0],ArrowUp:[0,-1],ArrowDown:[0,1]};
    if(!shifts[event.key]&&event.key!=='Home')return;
    event.preventDefault();const amount=event.shiftKey ? .05 : .01;
    if(event.key==='Home'){model.beam.x=.5;model.beam.y=.5;}
    else {model.beam.x=Math.max(0,Math.min(1,model.beam.x+shifts[event.key][0]*amount));model.beam.y=Math.max(0,Math.min(1,model.beam.y+shifts[event.key][1]*amount));}
    el('beam-x').value=model.beam.x*100;el('beam-y').value=model.beam.y*100;schedule();
  });
  el('physical-map-file').addEventListener('change',async event=>{
    const file=event.target.files[0];if(!file)return;
    const chip=state.chip.id;
    try {
      if(file.size>3*1024*1024)throw Error('Coordinate map must be under 3 MiB.');
      const text=await file.text();
      if(chip!==state.chip.id)throw Error('Chip changed while reading the map. Select the file again.');
      const map=validateMap(JSON.parse(text));
      model.custom=map;update();
      el('physical-map-status').textContent=`Loaded ${map.regions.length} ranges. Source: ${map.source}${map.model ? ' (illustrative model)' : ' (user-supplied coordinates)'}.`;
    } catch(error) {el('physical-map-status').textContent=`Map not loaded: ${error.message}`;}
    event.target.value='';
  });
  el('reset-physical-map').addEventListener('click',()=>{model.custom=null;el('physical-map-status').textContent='Grid model restored.';update();});
  el('export-physical-map').addEventListener('click',()=>{
    const url=URL.createObjectURL(new Blob([JSON.stringify(model.map,null,2)+'\n'],{type:'application/json'}));
    const link=document.createElement('a');link.href=url;link.download=`${state.chip.id}-coordinate-map.json`;link.click();
    setTimeout(()=>URL.revokeObjectURL(url),1000);
  });
  window.addEventListener('resize',schedule);
  // The hidden linear canvas needs a redraw when its details element opens.
  document.querySelector('.linear-flash').addEventListener('toggle',event=>{if(event.target.open)drawChip();});
  window.NiFATSpatial={update,intersects,validateMap,snapshot:()=>({map:model.map,copies:model.copies,
    exposed:[...model.exposed],beam:{...model.beam},cells:model.cells})};
  update();
})();
