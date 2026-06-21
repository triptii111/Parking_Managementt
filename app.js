const content = document.getElementById('content')
const toastsEl = document.getElementById('toasts')

async function get(path) {
  const res = await fetch('/api' + path)
  return res.json()
}

async function post(path, body) {
  const res = await fetch('/api' + path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body)
  })
  return res.json()
}

function toast(msg, type = 'info') {
  const el = document.createElement('div')
  el.className = `toast toast-${type}`
  el.textContent = msg
  toastsEl.appendChild(el)
  setTimeout(() => el.remove(), 3500)
}

function badge(text, type) {
  return `<span class="badge badge-${type}">${text}</span>`
}

function emptyState(icon, msg) {
  return `<div class="empty"><div class="empty-icon">${icon}</div><p>${msg}</p></div>`
}

document.getElementById('nav').addEventListener('click', function(e) {
  const link = e.target.closest('.nav-link')
  if (!link) return
  e.preventDefault()
  document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'))
  link.classList.add('active')
  showPage(link.dataset.page)
})

function showPage(page) {
  const pages = { dashboard, park, exit, slots, waiting, history: showHistory, search: searchPage, stats }
  if (pages[page]) pages[page]()
}

async function dashboard() {
  content.innerHTML = `
    <div class="page-top">
      <div class="page-header"><h1>Dashboard</h1><p>Real-time overview of the parking lot</p></div>
      <div style="display:flex;gap:10px">
        <button class="btn btn-ghost btn-sm" onclick="dashboard()">&#8635; Refresh</button>
        <button class="btn btn-ghost btn-sm" onclick="undo()">&#8617; Undo</button>
      </div>
    </div>
    <div id="dash-stats" class="stat-grid"></div>
    <div id="dash-bar" class="card"></div>
    <div id="dash-vehicles" class="card"></div>`

  const [s, v] = await Promise.all([get('/stats'), get('/vehicles')])

  const cards = [
    { label: 'Occupied', value: s.occupied, icon: '&#128308;', bg: '#FEE2E2', color: '#991B1B' },
    { label: 'Available', value: s.free, icon: '&#128994;', bg: '#DCFCE7', color: '#15803D' },
    { label: 'Waiting', value: s.waiting, icon: '&#128993;', bg: '#FEF3C7', color: '#92400E' },
    { label: 'Entries Today', value: s.totalToday, icon: '&#128197;', bg: '#DBEAFE', color: '#1E40AF' },
  ]

  document.getElementById('dash-stats').innerHTML = cards.map(c => `
    <div class="stat-card">
      <div class="stat-icon" style="background:${c.bg}">${c.icon}</div>
      <div><div class="stat-label">${c.label}</div><div class="stat-value" style="color:${c.color}">${c.value}</div></div>
    </div>`).join('')

  const pct = s.occupancyPercent
  const barColor = pct > 80 ? '#EF4444' : pct > 50 ? '#F59E0B' : '#10B981'
  document.getElementById('dash-bar').innerHTML = `
    <div style="display:flex;justify-content:space-between;margin-bottom:8px">
      <span style="font-weight:600">Occupancy</span>
      <span style="font-weight:700;color:${barColor}">${pct}%</span>
    </div>
    <div class="bar-wrap"><div class="bar-fill" style="width:${pct}%;background:${barColor}"></div></div>
    <div style="display:flex;justify-content:space-between;font-size:0.75rem;color:#94A3B8;margin-top:6px">
      <span>0</span><span>${s.totalSlots} total slots</span>
    </div>`

  const rows = v.length === 0 ? emptyState('&#127359;', 'No vehicles currently parked') : `
    <div class="table-wrap"><table>
      <thead><tr><th>Vehicle No.</th><th>Owner</th><th>Type</th><th>Slot</th><th>Status</th></tr></thead>
      <tbody>${v.slice(0, 10).map(vehicle => `
        <tr>
          <td style="font-weight:600">${vehicle.vehicleNumber}</td>
          <td>${vehicle.ownerName}</td>
          <td>${vehicle.vehicleType}</td>
          <td>${vehicle.slotNumber > 0 ? badge('#' + vehicle.slotNumber, 'info') : badge('Waiting', 'warning')}</td>
          <td>${vehicle.isParked && vehicle.slotNumber > 0 ? badge('Parked', 'success') : badge('In Queue', 'warning')}</td>
        </tr>`).join('')}
      </tbody>
    </table></div>`

  document.getElementById('dash-vehicles').innerHTML = `
    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px">
      <h2 style="font-size:1rem;font-weight:700">Currently Parked</h2>
    </div>${rows}`
}

async function undo() {
  const data = await post('/undo', {})
  toast(data.message, data.success ? 'success' : 'warning')
  if (data.success) dashboard()
}

function park() {
  content.innerHTML = `
    <div class="page-header"><h1>Park Vehicle</h1><p>Register a vehicle to enter the parking lot</p></div>
    <div class="two-col">
      <div class="card">
        <h2 style="font-size:1rem;font-weight:700;margin-bottom:20px">Vehicle Details</h2>
        <form id="park-form">
          <div class="form-group"><label>Vehicle Number *</label>
            <input id="vNum" placeholder="e.g. DL01AB1234" required style="text-transform:uppercase"></div>
          <div class="form-group"><label>Owner Name *</label>
            <input id="owner" placeholder="e.g. Tripti Kushwaha" required></div>
          <div class="form-group"><label>Vehicle Type</label>
            <select id="vType"><option>Car</option><option>Bike</option><option>Truck</option></select></div>
          <button class="btn btn-primary btn-full" type="submit">+ Park Vehicle</button>
        </form>
      </div>
      <div class="card"><h2 style="font-size:1rem;font-weight:700;margin-bottom:16px">Result</h2>
        <div id="park-result">${emptyState('&#127359;', 'Submit the form to park a vehicle')}</div>
      </div>
    </div>`

  document.getElementById('park-form').addEventListener('submit', async function(e) {
    e.preventDefault()
    const vNum = document.getElementById('vNum').value.trim().toUpperCase()
    const owner = document.getElementById('owner').value.trim()
    const type = document.getElementById('vType').value
    if (!vNum || !owner) { toast('Vehicle number and owner are required', 'warning'); return }

    const data = await post('/park', { vehicleNumber: vNum, owner, type })
    if (data.success && data.status === 'parked') {
      toast(data.message, 'success')
      document.getElementById('park-result').innerHTML = `
        <div class="result-box">
          <div class="result-icon">&#9989;</div>
          <div class="result-title" style="color:#15803D">Vehicle Parked!</div>
          <div class="slot-badge">Slot #${data.slot}</div>
        </div>`
      this.reset()
    } else if (data.success && data.status === 'waiting') {
      toast(data.message, 'warning')
      document.getElementById('park-result').innerHTML = `
        <div class="result-box">
          <div class="result-icon">&#9203;</div>
          <div class="result-title" style="color:#92400E">Added to Waiting Queue</div>
          <p style="color:#64748B;font-size:0.875rem">The lot is full. The vehicle will be assigned a slot automatically.</p>
        </div>`
      this.reset()
    } else {
      toast(data.message || 'Failed to park vehicle', 'error')
    }
  })
}

function exit() {
  content.innerHTML = `
    <div class="page-header"><h1>Exit Vehicle</h1><p>Search for a vehicle and process its exit</p></div>
    <div class="card" style="margin-bottom:20px">
      <h2 style="font-size:1rem;font-weight:700;margin-bottom:14px">Find Vehicle</h2>
      <div class="search-row">
        <input id="exit-query" placeholder="Enter vehicle number e.g. DL01AB1234" style="text-transform:uppercase;font-family:monospace">
        <button class="btn btn-primary" onclick="exitSearch()">&#128269; Search</button>
      </div>
    </div>
    <div id="exit-result"></div>`

  document.getElementById('exit-query').addEventListener('keydown', function(e) {
    if (e.key === 'Enter') exitSearch()
  })
}

async function exitSearch() {
  const q = document.getElementById('exit-query').value.trim().toUpperCase()
  if (!q) { toast('Enter a vehicle number', 'warning'); return }
  const data = await get('/search/' + encodeURIComponent(q))
  const el = document.getElementById('exit-result')
  if (!data.success) { toast('Vehicle not found', 'error'); el.innerHTML = ''; return }
  const v = data.vehicle
  el.innerHTML = `
    <div class="card">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px">
        <h2 style="font-size:1rem;font-weight:700">Vehicle Found</h2>
        ${badge('&#9679; Parked', 'success')}
      </div>
      <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:16px;margin-bottom:20px">
        ${[['Vehicle No.', v.vehicleNumber], ['Owner', v.ownerName], ['Type', v.vehicleType],
           ['Slot', v.slotNumber > 0 ? '#' + v.slotNumber : 'Waiting'], ['Entry', v.entryTime]]
          .map(([l, val]) => `<div><div class="field-label">${l}</div><div class="field-value">${val}</div></div>`).join('')}
      </div>
      <button class="btn btn-danger" onclick="confirmExit('${v.vehicleNumber}', ${v.slotNumber})">&#8594; Exit Vehicle</button>
    </div>`
}

function confirmExit(vNum, slot) {
  const overlay = document.createElement('div')
  overlay.className = 'overlay'
  overlay.innerHTML = `
    <div class="dialog">
      <h3>Confirm Exit</h3>
      <p>Are you sure you want to exit <strong>${vNum}</strong> from slot #${slot}?</p>
      <div class="dialog-btns">
        <button class="btn btn-ghost btn-sm" id="dlg-cancel">Cancel</button>
        <button class="btn btn-danger btn-sm" id="dlg-confirm">Exit Vehicle</button>
      </div>
    </div>`
  document.body.appendChild(overlay)
  document.getElementById('dlg-cancel').onclick = () => overlay.remove()
  document.getElementById('dlg-confirm').onclick = async () => {
    overlay.remove()
    const data = await post('/exit', { vehicleNumber: vNum })
    if (data.success) {
      toast(`${vNum} exited successfully`, 'success')
      document.getElementById('exit-result').innerHTML = `
        <div class="card" style="text-align:center;padding:40px">
          <div style="font-size:3rem;margin-bottom:12px">&#9989;</div>
          <div style="font-weight:700;font-size:1.1rem;color:#15803D;margin-bottom:8px">Exit Recorded</div>
          <p style="color:#64748B">${vNum} has exited. Slot is now available.</p>
        </div>`
    } else {
      toast(data.message || 'Failed', 'error')
    }
  }
}

async function slots() {
  content.innerHTML = `
    <div class="page-top">
      <div class="page-header"><h1>Parking Slots</h1><p>Live slot occupancy</p></div>
      <button class="btn btn-ghost btn-sm" onclick="slots()">&#8635; Refresh</button>
    </div>
    <div id="slots-content"></div>`

  const data = await get('/slots')
  const occupied = data.filter(s => s.occupied).length
  const free = data.length - occupied

  document.getElementById('slots-content').innerHTML = `
    <div style="display:flex;gap:16px;margin-bottom:16px;flex-wrap:wrap">
      <div style="display:flex;align-items:center;gap:6px;font-size:0.85rem">
        <div style="width:16px;height:16px;background:#DCFCE7;border-radius:4px;border:1px solid #86EFAC"></div>
        Available (${free})
      </div>
      <div style="display:flex;align-items:center;gap:6px;font-size:0.85rem">
        <div style="width:16px;height:16px;background:#FEE2E2;border-radius:4px;border:1px solid #FCA5A5"></div>
        Occupied (${occupied})
      </div>
    </div>
    <div class="card">
      <div class="slot-grid">
        ${data.map(s => `
          <div class="slot ${s.occupied ? 'occupied' : 'free'}">
            <span class="slot-num">${s.slotNumber}</span>
            ${s.occupied ? 'BUSY' : 'FREE'}
          </div>`).join('')}
      </div>
    </div>`
}

async function waiting() {
  content.innerHTML = `
    <div class="page-top">
      <div class="page-header"><h1>Waiting Queue</h1><p>Vehicles waiting for a slot</p></div>
      <button class="btn btn-ghost btn-sm" onclick="waiting()">&#8635; Refresh</button>
    </div>
    <div id="waiting-content"></div>`

  const data = await get('/waiting')
  const el = document.getElementById('waiting-content')

  if (data.length === 0) {
    el.innerHTML = `<div class="card">${emptyState('&#9989;', 'No vehicles in the waiting queue')}</div>`
    return
  }

  el.innerHTML = `
    <div class="card">
      <p style="margin-bottom:12px;color:#64748B"><strong>${data.length}</strong> vehicle${data.length !== 1 ? 's' : ''} waiting</p>
      ${data.map(item => `
        <div class="queue-item">
          <div class="queue-pos">${item.position}</div>
          <div style="flex:1">
            <div style="font-weight:600;font-family:monospace">${item.vehicleNumber}</div>
            <div style="font-size:0.78rem;color:#64748B">
              ${item.position === 1 ? 'Next in line — will get next free slot' : 'Position ' + item.position + ' in queue'}
            </div>
          </div>
          ${item.position === 1 ? badge('Next', 'warning') : ''}
        </div>`).join('')}
    </div>`
}

let historyData = []
let historySort = 'time'

async function showHistory() {
  content.innerHTML = `
    <div class="page-top">
      <div class="page-header"><h1>Parking History</h1><p>Complete log of all vehicle exits</p></div>
      <button class="btn btn-ghost btn-sm" onclick="showHistory()">&#8635; Refresh</button>
    </div>
    <div class="card">
      <input id="hist-search" placeholder="Search by vehicle number or owner..." style="margin-bottom:12px">
      <div class="sort-bar">
        <span>Sort by:</span>
        <button onclick="setHistSort('time')" class="${historySort==='time'?'active':''}">Entry Time</button>
        <button onclick="setHistSort('slot')" class="${historySort==='slot'?'active':''}">Slot No.</button>
        <button onclick="setHistSort('vehicle')" class="${historySort==='vehicle'?'active':''}">Vehicle No.</button>
      </div>
      <div id="hist-table"></div>
    </div>`

  const data = await get('/history')
  historyData = data
  renderHistoryTable()

  document.getElementById('hist-search').addEventListener('input', renderHistoryTable)
}

function setHistSort(key) {
  historySort = key
  showHistory()
}

function clientMergeSort(arr, key) {
  if (arr.length <= 1) return arr
  const mid = Math.floor(arr.length / 2)
  const left = clientMergeSort(arr.slice(0, mid), key)
  const right = clientMergeSort(arr.slice(mid), key)
  const result = []
  let i = 0, j = 0
  while (i < left.length && j < right.length) {
    const lv = key === 'slot' ? left[i].slotNumber : key === 'vehicle' ? left[i].vehicleNumber : left[i].entryTime
    const rv = key === 'slot' ? right[j].slotNumber : key === 'vehicle' ? right[j].vehicleNumber : right[j].entryTime
    if (lv <= rv) { result.push(left[i]); i++ } else { result.push(right[j]); j++ }
  }
  while (i < left.length) { result.push(left[i]); i++ }
  while (j < right.length) { result.push(right[j]); j++ }
  return result
}

function renderHistoryTable() {
  const query = (document.getElementById('hist-search')?.value || '').trim().toUpperCase()
  let data = historyData.filter(r =>
    !query || r.vehicleNumber.includes(query) || r.ownerName.toUpperCase().includes(query)
  )
  data = clientMergeSort(data, historySort)

  const el = document.getElementById('hist-table')
  if (!el) return

  if (data.length === 0) {
    el.innerHTML = emptyState('&#128203;', query ? 'No records match your search' : 'No parking history yet')
    return
  }

  el.innerHTML = `
    <div class="table-wrap"><table>
      <thead><tr><th>#</th><th>Vehicle No.</th><th>Owner</th><th>Type</th><th>Slot</th><th>Entry Time</th><th>Exit Time</th></tr></thead>
      <tbody>${data.map((r, idx) => `
        <tr>
          <td style="color:#94A3B8;font-size:0.8rem">${idx + 1}</td>
          <td style="font-weight:600">${r.vehicleNumber}</td>
          <td>${r.ownerName}</td>
          <td>${badge(r.vehicleType, 'muted')}</td>
          <td>${badge('#' + r.slotNumber, 'info')}</td>
          <td style="color:#64748B;font-size:0.8rem">${r.entryTime}</td>
          <td style="color:#64748B;font-size:0.8rem">${r.exitTime}</td>
        </tr>`).join('')}
      </tbody>
    </table></div>`
}

function searchPage() {
  content.innerHTML = `
    <div class="page-header"><h1>Search Vehicle</h1><p>Find a vehicle by its registration number</p></div>
    <div class="card" style="margin-bottom:20px">
      <div class="search-row">
        <input id="search-query" placeholder="Enter exact vehicle number e.g. DL01AB1234" style="text-transform:uppercase;font-family:monospace;font-size:1rem">
        <button class="btn btn-primary" onclick="doSearch()">&#128269; Search</button>
      </div>
    </div>
    <div id="search-result"></div>`

  document.getElementById('search-query').addEventListener('keydown', e => {
    if (e.key === 'Enter') doSearch()
  })
}

async function doSearch() {
  const q = document.getElementById('search-query').value.trim().toUpperCase()
  if (!q) { toast('Enter a vehicle number to search', 'warning'); return }
  const data = await get('/search/' + encodeURIComponent(q))
  const el = document.getElementById('search-result')
  if (!data.success) {
    el.innerHTML = `<div class="card">${emptyState('&#128269;', 'No vehicle found with number <strong>' + q + '</strong>')}</div>`
    return
  }
  const v = data.vehicle
  el.innerHTML = `
    <div class="card">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:4px">
        <h2 style="font-size:1.1rem;font-weight:700">${v.vehicleNumber}</h2>
        ${v.isParked && v.slotNumber > 0 ? badge('&#9679; Parked', 'success') : badge('&#9203; In Queue', 'warning')}
      </div>
      ${[['Owner', v.ownerName], ['Vehicle Type', v.vehicleType],
         ['Slot Number', v.slotNumber > 0 ? '#' + v.slotNumber : 'Waiting in queue'],
         ['Entry Time', v.entryTime], ['Exit Time', v.exitTime || 'Still parked']]
        .map(([l, val]) => `
          <div class="field-row">
            <div class="field-label">${l}</div>
            <div class="field-value">${val}</div>
          </div>`).join('')}
    </div>`
}

async function stats() {
  content.innerHTML = `
    <div class="page-top">
      <div class="page-header"><h1>Statistics</h1><p>Parking analytics</p></div>
      <button class="btn btn-ghost btn-sm" onclick="stats()">&#8635; Refresh</button>
    </div>
    <div id="stats-content"></div>`

  const [s, v] = await Promise.all([get('/stats'), get('/vehicles')])

  const typeCounts = {}
  v.forEach(vehicle => { typeCounts[vehicle.vehicleType] = (typeCounts[vehicle.vehicleType] || 0) + 1 })
  const typeColors = { Car: '#2563EB', Bike: '#10B981', Truck: '#F59E0B' }
  const total = v.length || 1
  const typeSegments = Object.entries(typeCounts).map(([type, count]) => ({
    label: type, count, pct: count / total, color: typeColors[type] || '#94A3B8'
  }))
  const occ = s.occupied || 0
  const fr = s.free || 0
  const ts = (occ + fr) || 1
  const occSegments = [
    { label: 'Occupied', count: occ, pct: occ / ts, color: '#EF4444' },
    { label: 'Available', count: fr, pct: fr / ts, color: '#10B981' }
  ]

  function makeSvg(segments) {
    let cumulative = 0
    const size = 160
    const r = size / 2 - 10
    const cx = size / 2, cy = size / 2
    const paths = segments.map(seg => {
      const start = cumulative * 2 * Math.PI - Math.PI / 2
      cumulative += seg.pct
      const end = cumulative * 2 * Math.PI - Math.PI / 2
      const large = seg.pct > 0.5 ? 1 : 0
      const x1 = cx + r * Math.cos(start), y1 = cy + r * Math.sin(start)
      const x2 = cx + r * Math.cos(end), y2 = cy + r * Math.sin(end)
      return `<path d="M${cx},${cy} L${x1},${y1} A${r},${r},0,${large},1,${x2},${y2}Z" fill="${seg.color}" stroke="#fff" stroke-width="2"/>`
    }).join('')
    return `<svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">${paths}</svg>`
  }

  document.getElementById('stats-content').innerHTML = `
    <div class="stat-grid" style="margin-bottom:20px">
      ${[
        ['Total Slots', s.totalSlots, '#1E40AF'],
        ['Occupied', s.occupied, '#991B1B'],
        ['Available', s.free, '#15803D'],
        ['Waiting', s.waiting, '#92400E'],
        ['Entries Today', s.totalToday, '#7C3AED'],
        ['Occupancy', s.occupancyPercent + '%', '#15803D']
      ].map(([l, val, color]) => `
        <div class="stat-card">
          <div style="flex:1"><div class="stat-label">${l}</div>
          <div class="stat-value" style="color:${color}">${val}</div></div>
        </div>`).join('')}
    </div>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-bottom:20px">
      <div class="card">
        <h2 style="font-size:1rem;font-weight:700;margin-bottom:16px">Slot Occupancy</h2>
        <div class="pie-row">
          ${makeSvg(occSegments)}
          <ul class="pie-legend">${occSegments.map(s => `
            <li><div class="dot" style="background:${s.color}"></div>
            <span>${s.label} (${s.count})</span></li>`).join('')}
          </ul>
        </div>
      </div>
      <div class="card">
        <h2 style="font-size:1rem;font-weight:700;margin-bottom:16px">Vehicle Types</h2>
        ${typeSegments.length === 0 ? emptyState('', 'No vehicles parked') : `
        <div class="pie-row">
          ${makeSvg(typeSegments)}
          <ul class="pie-legend">${typeSegments.map(s => `
            <li><div class="dot" style="background:${s.color}"></div>
            <span>${s.label} (${s.count})</span></li>`).join('')}
          </ul>
        </div>`}
      </div>
    </div>
  `
}

dashboard()
