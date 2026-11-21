(() => {
  'use strict';
  document.querySelectorAll('input[id], select[id]').forEach(input => {
    const label = input.closest('.ctl')?.querySelector('label');
    if (label && !input.labels?.length) label.htmlFor = input.id;
  });
  document.querySelectorAll('table').forEach(table => {
    if (table.closest('.table-wrap')) return;
    const wrap = document.createElement('div');
    wrap.className = 'table-wrap';
    wrap.tabIndex = 0;
    wrap.setAttribute('role', 'region');
    wrap.setAttribute('aria-label', 'Scrollable reference table');
    table.before(wrap); wrap.append(table);
  });
  document.querySelectorAll('canvas').forEach(canvas => {
    if (canvas.id === 'spatialcv') return;
    canvas.setAttribute('role', 'img');
    canvas.setAttribute('aria-label', canvas.id === 'chipcv' ? 'Flash partition map. See the legend and chip details for values.' : 'Volume sector map. See the scattered structures table for sector addresses.');
  });
  const stage = document.querySelector('.stage');
  if (stage) {
    stage.tabIndex = 0;
    stage.setAttribute('role', 'region');
    stage.setAttribute('aria-label', 'Interactive filesystem diagram. Scroll horizontally to explore.');
  }
  if (window.parent === window) {
    const back = document.createElement('a');
    back.className = 'standalone-back';
    back.href = '../index.html#/';
    back.textContent = 'NiFAT32 / Back to overview';
    document.body.prepend(back);
    return;
  }
  let pending = false;
  function resize() {
    if (pending) return;
    pending = true;
    requestAnimationFrame(() => {
      pending = false;
      window.parent.postMessage({type:'nifat32:resize', height:document.body.getBoundingClientRect().height}, location.origin === 'null' ? '*' : location.origin);
    });
  }
  new ResizeObserver(resize).observe(document.body);
  resize();
})();
