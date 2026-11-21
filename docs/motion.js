/* Decorative motion only: no remote assets or animation dependencies. */
(() => {
  'use strict';
  const root = document.documentElement;
  const scene = document.querySelector('.hero-visual');
  const svg = document.getElementById('hero-mars-scene');
  if (!scene || !svg) return;
  const front = svg.querySelector('#satellite-front');
  const back = svg.querySelector('#satellite-back');
  if (!front || !back) return;

  const reduced = matchMedia('(prefers-reduced-motion: reduce)');
  let sceneVisible = false;
  let frame = 0;
  let lastTime = 0;
  let elapsed = 0;

  function positionSatellite() {
    // Same ellipse as the drawn orbital guide, rotated -32 degrees. Two copies
    // sandwich the planet/chip in SVG paint order for genuine front/back passes.
    const theta = -.52 + elapsed / 32000 * Math.PI * 2;
    const tilt = -32 * Math.PI / 180;
    const depth = Math.sin(theta);
    const x = 319 + 294 * Math.cos(theta) * Math.cos(tilt) - 120 * depth * Math.sin(tilt);
    const y = 314 + 294 * Math.cos(theta) * Math.sin(tilt) + 120 * depth * Math.cos(tilt);
    const transform = `translate(${x.toFixed(3)} ${y.toFixed(3)}) rotate(${(26 + depth * 12).toFixed(2)}) scale(${(.92 + depth * .12).toFixed(3)})`;
    for (const satellite of [front, back]) {
      satellite.setAttribute('transform', transform);
      satellite.setAttribute('opacity', String(.88 + depth * .12));
    }
    front.setAttribute('visibility', depth >= 0 ? 'visible' : 'hidden');
    back.setAttribute('visibility', depth < 0 ? 'visible' : 'hidden');
  }

  function canAnimate() { return !reduced.matches && !document.hidden && sceneVisible; }
  function tick(time) {
    frame = 0;
    if (!canAnimate()) return;
    if (lastTime) elapsed += Math.min(time - lastTime, 80);
    lastTime = time;
    positionSatellite();
    frame = requestAnimationFrame(tick);
  }
  function sync() {
    const paused = reduced.matches;
    root.dataset.motion = paused ? 'paused' : 'running';
    root.classList.toggle('motion-background', document.hidden);
    scene.dataset.scenePaused = String(!canAnimate());
    if (canAnimate()) {
      if (!frame) { lastTime = 0; frame = requestAnimationFrame(tick); }
    } else {
      cancelAnimationFrame(frame);
      frame = 0;
      lastTime = 0;
    }
  }
  reduced.addEventListener('change', sync);
  document.addEventListener('visibilitychange', sync);
  window.addEventListener('pagehide', () => { cancelAnimationFrame(frame); frame = 0; lastTime = 0; });
  window.addEventListener('pageshow', sync);

  if ('IntersectionObserver' in window) {
    const sceneObserver = new IntersectionObserver(entries => {
      sceneVisible = entries[0].isIntersecting;
      sync();
    }, {threshold:0});
    sceneObserver.observe(scene);

    // Content is visible by default. Only opt into reveal styling after the
    // observer exists; without JS (or during reduced motion), nothing is hidden.
    const groups = [
      '.section-heading', '.protection-grid', '.lab-grid', '.explore-grid', '.start-section', '.spec-inner', '.research-grid', '.footprint-charts'
    ];
    const targets = new Set(document.querySelectorAll('.compatibility-note'));
    for (const selector of groups) {
      for (const group of document.querySelectorAll(selector)) {
        [...group.children].forEach((child, index) => {
          child.style.setProperty('--reveal-delay', `${Math.min(index, 3) * 85}ms`);
          targets.add(child);
        });
      }
    }
    const reveals = new IntersectionObserver(entries => {
      for (const entry of entries) {
        if (!entry.isIntersecting) continue;
        entry.target.classList.add('is-visible');
        reveals.unobserve(entry.target);
      }
    }, {threshold:.08});
    for (const target of targets) {
      target.dataset.reveal = '';
      reveals.observe(target);
    }
    root.classList.add('motion-ready');

    const loops = new IntersectionObserver(entries => {
      for (const entry of entries) entry.target.dataset.motionVisible = String(entry.isIntersecting);
    });
    document.querySelectorAll('.mini-map, .mini-flow').forEach(element => {
      element.dataset.motionVisible = 'false';
      loops.observe(element);
    });
  } else {
    const measure = () => {
      const box = scene.getBoundingClientRect();
      sceneVisible = box.width > 0 && box.bottom > 0 && box.top < innerHeight;
      sync();
    };
    for (const event of ['scroll', 'resize', 'hashchange']) window.addEventListener(event, measure, {passive:true});
    measure();
  }
  const vote = document.querySelector('.vote-demo');
  document.getElementById('fault-button')?.addEventListener('click', () => vote?.classList.remove('just-repaired'));
  document.getElementById('repair-button')?.addEventListener('click', () => vote?.classList.add('just-repaired'));
  positionSatellite();
  sync();
})();
