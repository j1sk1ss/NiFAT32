/* Local navigation, full-text search, and a deliberately simplified voting demo. */
(() => {
  'use strict';
  const $ = (selector, root = document) => root.querySelector(selector);
  const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
  const pages = window.NIFAT32_DOCS || {};
  const entries = Object.entries(pages);
  const escapeHTML = value => value.replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const home = $('#home-page');
  const docs = $('#docs-page');
  const article = $('#document-content');
  let currentRoute = null;
  let toastTimer;

  function toast(message) {
    const el = $('#toast');
    el.textContent = message;
    el.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { el.hidden = true; }, 2800);
  }

  function closeMenus() {
    $('.main-nav').classList.remove('open');
    $('.menu-toggle').setAttribute('aria-expanded', 'false');
    $('.menu-toggle').setAttribute('aria-label', 'Open navigation');
    $('.docs-sidebar').classList.remove('open');
    $('.docs-menu-button').setAttribute('aria-expanded', 'false');
  }
  $('.menu-toggle').addEventListener('click', event => {
    const open = $('.main-nav').classList.toggle('open');
    event.currentTarget.setAttribute('aria-expanded', String(open));
    event.currentTarget.setAttribute('aria-label', open ? 'Close navigation' : 'Open navigation');
  });
  $('.docs-menu-button').addEventListener('click', event => {
    event.currentTarget.setAttribute('aria-expanded', String($('.docs-sidebar').classList.toggle('open')));
  });

  let group = '';
  $('#docs-sidebar').innerHTML = entries.map(([path, page]) => {
    const heading = page.group === group ? '' : `<p class="sidebar-group">${page.group.toUpperCase()}</p>`;
    group = page.group;
    return `${heading}<a href="#/${path}" data-route="${path}">${page.title}</a>`;
  }).join('') + '<div class="sidebar-note"><p>Planning your flash layout?</p><a href="#/builder">Try the filesystem builder</a></div>';

  function route(initial = false) {
    const raw = location.hash.slice(1) || '/';
    if (!raw.startsWith('/')) return; // Native skip-to-content anchors.
    const [pathname, query = ''] = raw.split('?');
    const path = pathname.replace(/^\/+|\/+$/g, '').replace(/\.md$/, '').replace(/^index$/, '');
    const anchor = new URLSearchParams(query).get('id');
    closeMenus();
    const changed = currentRoute !== path;
    const isHome = path === '';
    home.hidden = !isHome;
    docs.hidden = isHome;
    docs.classList.toggle('tool-layout', path === 'builder' || path === 'animations');
    $$('[data-nav]').forEach(link => {
      const active = link.dataset.nav === (isHome ? 'home' : path.startsWith('docs/') ? 'docs' : path);
      if (active) link.setAttribute('aria-current', 'page'); else link.removeAttribute('aria-current');
    });
    $$('[data-route]').forEach(link => {
      if (link.dataset.route === path) link.setAttribute('aria-current', 'page'); else link.removeAttribute('aria-current');
    });
    if (isHome) {
      document.title = 'NiFAT32 — Resilience at speed';
    } else if (changed) {
      const page = pages[path];
      if (!page) {
        article.innerHTML = '<div class="error-page"><p class="eyebrow">PAGE NOT FOUND</p><h1>Uncharted territory.</h1><p>This documentation page could not be found.</p><a href="#/docs/getting-started">Go to getting started</a></div>';
        $('#page-outline').replaceChildren();
        $('#page-pagination').replaceChildren();
        $('#breadcrumb').textContent = 'DOCUMENTATION / PAGE NOT FOUND';
        $('#edit-source').hidden = true;
        document.title = 'Page not found — NiFAT32';
      } else {
        article.innerHTML = page.html;
        $('#edit-source').hidden = false;
        $('#breadcrumb').textContent = `${page.group.toUpperCase()} / ${page.title.toUpperCase()}`;
        $('#edit-source').href = `https://github.com/j1sk1ss/NIFAT32/blob/HEAD/docs/${page.source}`;
        document.title = `${page.title} — NiFAT32`;
        $('#page-outline').innerHTML = page.headings.map(h => `<a href="#/${path}?id=${encodeURIComponent(h.id)}"${h.level === 3 ? ' style="padding-left:10px"' : ''}>${escapeHTML(h.title)}</a>`).join('');
        $$('h2[id], h3[id]', article).forEach(heading => {
          const a = document.createElement('a');
          a.className = 'heading-anchor';
          a.href = `#/${path}?id=${encodeURIComponent(heading.id)}`;
          a.setAttribute('aria-label', `Link to ${heading.textContent}`);
          a.textContent = '#';
          heading.append(a);
        });
        $$('pre', article).forEach((pre, index) => {
          const code = $('code', pre);
          if (!code) return;
          code.id = `code-block-${index}`;
          const button = document.createElement('button');
          button.className = 'copy-button';
          button.dataset.copy = code.id;
          button.textContent = 'Copy';
          button.setAttribute('aria-label', 'Copy code block');
          pre.append(button);
        });
        const index = entries.findIndex(([key]) => key === path);
        $('#page-pagination').innerHTML = [index > 0 ? entries[index - 1] : null, entries[index + 1] || null].map((entry, i) => entry ? `<a href="#/${entry[0]}" class="${i ? 'next' : 'previous'}"><small>${i ? 'NEXT' : 'PREVIOUS'}</small>${entry[1].title}</a>` : '').join('');
      }
    }
    currentRoute = path;
    if (anchor) {
      requestAnimationFrame(() => {
        const target = document.getElementById(anchor);
        if (target) {
          target.scrollIntoView({block:'start', behavior:'instant'});
          target.tabIndex = -1;
          target.focus({preventScroll:true});
        }
        $$('#page-outline a').forEach(link => {
          if (link.hash === location.hash) link.setAttribute('aria-current', 'location');
          else link.removeAttribute('aria-current');
        });
      });
    } else if (changed && !initial) {
      window.scrollTo({top:0, behavior:'instant'});
      const focus = isHome ? $('#main') : $('h1', article);
      if (focus) { focus.tabIndex = -1; focus.focus({preventScroll:true}); }
    }
  }
  window.addEventListener('hashchange', () => route());
  route(true);

  document.addEventListener('change', event => {
    if (event.target.matches('[data-throughput-comparison]')) {
      event.target.closest('.comparison-chart').querySelectorAll('[data-comparison]').forEach(panel => {
        panel.hidden = panel.dataset.comparison !== event.target.value;
      });
      return;
    }
    if (!event.target.matches('[data-fault-volume]')) return;
    const chart = event.target.closest('.research-chart');
    chart.querySelectorAll('[data-volume]').forEach(figure => {
      figure.hidden = figure.dataset.volume !== event.target.value;
    });
  });

  document.addEventListener('click', async event => {
    const button = event.target.closest('[data-copy]');
    if (!button) return;
    const text = document.getElementById(button.dataset.copy)?.textContent;
    if (!text) return;
    try {
      if (navigator.clipboard && window.isSecureContext) await navigator.clipboard.writeText(text);
      else {
        const field = document.createElement('textarea');
        field.value = text; field.style.cssText = 'position:fixed;left:-9999px;top:0';
        document.body.append(field); field.select();
        const success = document.execCommand('copy');
        field.remove(); button.focus();
        if (!success) throw new Error('Clipboard unavailable');
      }
      toast('Code copied to clipboard');
      button.textContent = 'Copied';
      setTimeout(() => { button.textContent = 'Copy'; }, 2000);
    } catch (_) { toast('Select the code and copy it with Ctrl/Cmd+C'); }
  });

  const dialog = $('#search-dialog');
  const input = $('#search-input');
  const results = $('#search-results');
  function search() {
    const term = input.value.trim().toLowerCase();
    const words = term.split(/\s+/).filter(Boolean);
    const found = entries.map(([path, page]) => {
      const title = page.title.toLowerCase(), text = page.text.toLowerCase();
      const score = words.every(word => text.includes(word) || title.includes(word))
        ? words.reduce((sum, word) => sum + (title.includes(word) ? 20 : 0) + Math.min(5, text.split(word).length - 1), 0) : -1;
      return {path, page, score};
    }).filter(item => item.score >= 0).sort((a,b) => b.score - a.score);
    if (!found.length) {
      results.innerHTML = '<p class="search-empty">No matching guides. Try “mount”, “FAT”, or “Hamming”.</p>';
      return;
    }
    function highlight(text) {
      if (!words.length) return escapeHTML(text);
      const pattern = new RegExp('(' + words.map(word => word.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')).join('|') + ')', 'gi');
      return text.split(pattern).map(part => words.includes(part.toLowerCase()) ? `<mark>${escapeHTML(part)}</mark>` : escapeHTML(part)).join('');
    }
    results.innerHTML = found.map(({path,page}) => {
      let position = term ? page.text.toLowerCase().indexOf(words[0]) : 0;
      const start = Math.max(0, position - 45);
      const excerpt = (start ? '…' : '') + page.text.slice(start, start + 145) + '…';
      return `<a class="search-result" href="#/${path}"><strong>${highlight(page.title)}</strong><p>${highlight(excerpt)}</p></a>`;
    }).join('');
  }
  function openSearch() {
    if (dialog.open) return;
    dialog.showModal();
    search(); input.focus();
  }
  $('.search-trigger').addEventListener('click', openSearch);
  $('.close-search').addEventListener('click', () => dialog.close());
  input.addEventListener('input', search);
  results.addEventListener('click', event => { if (event.target.closest('a')) dialog.close(); });
  dialog.addEventListener('click', event => {
    if (event.target !== dialog) return;
    const rect = dialog.getBoundingClientRect();
    if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) dialog.close();
  });
  document.addEventListener('keydown', event => {
    const editable = event.target instanceof Element && event.target.matches('input, textarea, select, [contenteditable="true"]');
    if ((event.key === '/' && !editable) || ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'k')) {
      event.preventDefault(); openSearch();
    }
    if (!dialog.open) return;
    const links = $$('.search-result', results);
    if (event.key === 'ArrowDown' || event.key === 'ArrowUp') {
      event.preventDefault();
      const index = links.indexOf(document.activeElement);
      const next = event.key === 'ArrowDown' ? index + 1 : index - 1;
      if (next < 0) input.focus(); else links[Math.min(next, links.length - 1)]?.focus();
    }
    if (event.key === 'Enter' && document.activeElement === input && links[0]) {
      event.preventDefault(); links[0].click();
    }
  });

  function renderCopies(corrupted) {
    $('#copy-rows').innerHTML = Array.from({length:5}, (_,i) => {
      const bad = corrupted && i === 2;
      const value = bad ? 0x0B : 0x0A;
      return `<div class="copy-row${bad ? ' corrupted' : ''}"><span class="copy-name">FAT ${i + 1}</span><code>0x${value.toString(16).padStart(8,'0').toUpperCase()}</code><span class="bit-cells" aria-hidden="true">${value.toString(2).padStart(8,'0').split('').map(bit => `<i class="${bit === '1' ? 'on' : ''}"></i>`).join('')}</span><span class="copy-state">${bad ? 'UPSET' : 'AGREES'}</span></div>`;
    }).join('');
  }
  renderCopies(false);
  $('#fault-button').addEventListener('click', () => {
    renderCopies(true);
    $('#vote-status').textContent = 'One bit flipped in FAT 3. Four copies still agree on 0x0000000A.';
    $('#vote-status').classList.add('corrupted');
    $('#repair-button').disabled = false;
    $('#fault-button').disabled = true;
  });
  $('#repair-button').addEventListener('click', () => {
    renderCopies(false);
    $('#vote-status').textContent = 'Majority wins. All five copies restored to 0x0000000A.';
    $('#vote-status').classList.remove('corrupted');
    $('#fault-button').disabled = false;
    $('#repair-button').disabled = true;
  });

  // Only accept size messages from the currently embedded, local tool.
  window.addEventListener('message', event => {
    const frame = $('iframe.embed', article);
    if (!frame || event.source !== frame.contentWindow || event.data?.type !== 'nifat32:resize') return;
    if (event.origin !== location.origin) return;
    const height = Number(event.data.height);
    if (Number.isFinite(height) && height > 100 && height < 20000) frame.style.height = `${Math.ceil(height) + 4}px`;
  });
})();
