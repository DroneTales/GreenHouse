// =============================================================================
//  Global variables.

let allSensors = [];
let toggleableSensors = [];
let fixedSensorIds = new Set();
let enabledToggleIds = new Set();

let chart = null;
let lastFetchedData = [];

// =============================================================================


// =============================================================================
// Date / Time Helpers.

function dateToUnixStart(dateStr)
{
    const d = new Date(dateStr + 'T00:00:00');
    return Math.floor(d.getTime() / 1000);
}

function dateToUnixEnd(dateStr)
{
    const d = new Date(dateStr + 'T23:59:59');
    return Math.floor(d.getTime() / 1000);
}

function getTodayStr()
{
    const now = new Date();
    return formatDate(now);
}

/**
 * Formats a Date object as YYYY-MM-DD (local time).
 * @param {Date} d
 * @returns {string}
 */
function formatDate(d)
{
    const year = d.getFullYear();
    const month = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
}

// =============================================================================


// =============================================================================
// API Calls.

async function fetchSensors()
{
    const resp = await fetch('/api/sensors');
    if (!resp.ok)
        throw new Error('Failed to fetch sensors');

    allSensors = await resp.json();
    toggleableSensors = allSensors.filter(s => s.id < 100);
    fixedSensorIds = new Set(allSensors.filter(s => s.id >= 100).map(s => s.id));
    enabledToggleIds = new Set();
    createToggleCheckboxes();
}

function createToggleCheckboxes()
{
    const container = document.getElementById('toggleable-sensors');
    container.innerHTML = '';
    toggleableSensors.forEach(sensor =>
        {
            const label = document.createElement('label');
            label.style.marginRight = '15px';
            const cb = document.createElement('input');
            cb.type = 'checkbox';
            cb.value = sensor.id;
            cb.checked = false;
            cb.addEventListener('change', () =>
                {
                    if (cb.checked)
                        enabledToggleIds.add(sensor.id);
                    else
                        enabledToggleIds.delete(sensor.id);
                    rebuildChartWithCurrentData();
                }
            );
            label.appendChild(cb);
            label.appendChild(document.createTextNode(' ' + sensor.name));
            container.appendChild(label);
        }
    );
}

function getActiveSensorIds()
{
    const active = new Set([...fixedSensorIds]);
    for (let id of enabledToggleIds)
        active.add(id);
    return active;
}

async function fetchData(startUnix, endUnix)
{
    const url = `/api/data?start=${startUnix}&end=${endUnix}`;
    const resp = await fetch(url);
    if (!resp.ok)
        throw new Error('Failed to fetch data');
    return resp.json();
}

// =============================================================================


// =============================================================================
// Chart Building.

function buildDatasets(dataArray, activeSensorIds, sensorMap)
{
    const groups = {};
    dataArray.forEach(point =>
        {
            if (!activeSensorIds.has(point.sensor_id))
                return;
        
            const sid = point.sensor_id;
            if (!groups[sid])
                groups[sid] = [];
            
            groups[sid].push(
                {
                    x: point.date_time * 1000,
                    y: point.value
                }
            );
        }
    );

    const colors = [
        '#FF6384', '#36A2EB', '#FFCE56', '#4BC0C0', '#9966FF',
        '#FF9F40', '#E7E9ED', '#76B041', '#D19A66', '#9B59B6'
    ];

    const datasets = [];
    let colorIdx = 0;
    for (const [sid, points] of Object.entries(groups))
    {
        points.sort((a, b) => a.x - b.x);
        const sensorName = sensorMap[sid] || `Sensor ${sid}`;
        datasets.push(
            {
                label: sensorName,
                data: points,
                borderColor: colors[colorIdx % colors.length],
                backgroundColor: 'transparent',
                pointRadius: 1,
                borderWidth: 2,
                tension: 0.1
            }
        );
        colorIdx++;
    }
    return datasets;
}

function rebuildChartWithCurrentData()
{
    if (!lastFetchedData.length)
        return;

    const sensorMap = {};
    allSensors.forEach(s => { sensorMap[s.id] = s.name; });
    const activeIds = getActiveSensorIds();
    const datasets = buildDatasets(lastFetchedData, activeIds, sensorMap);

    const ctx = document.getElementById('sensorChart').getContext('2d');
    if (chart)
        chart.destroy();

    chart = new Chart(ctx,
        {
            type: 'line',
            data:
            {
                datasets
            },
            options:
            {
                responsive: true,
                maintainAspectRatio: false,
                scales:
                {
                    x:
                    {
                        type: 'time',
                        time:
                        {
                            unit: 'minute',
                            tooltipFormat: 'yyyy-MM-dd HH:mm:ss',
                            displayFormats:
                            {
                                minute: 'HH:mm',
                                hour: 'HH:mm',
                                day: 'MMM dd'
                            }
                        },
                        title:
                        {
                            display: true,
                            text: 'Time'
                        }
                    },
                    y:
                    {
                        title:
                        {
                            display: true,
                            text: 'Value'
                        }
                    }
                },
                plugins:
                {
                    tooltip:
                    {
                        mode: 'index',
                        intersect: false
                    },
                    legend:
                    {
                        position: 'top'
                    }
                }
            }
        }
    );
}

// =============================================================================


// =============================================================================
// UI Update Helpers.

function updateRangeLabel(text)
{
    document.getElementById('range-label').textContent = text;
}

async function updateChart(startUnix, endUnix, labelText)
{
    try
    {
        const data = await fetchData(startUnix, endUnix);
        lastFetchedData = data;
        rebuildChartWithCurrentData();
        if (labelText)
            updateRangeLabel(labelText);
    }
    catch(err)
    {
        console.error('Chart update error:', err);
        alert('Failed to load chart data. Details in console (F12).');
    }
}

function setActivePresetButton(activeId)
{
    const presetIds = ['yesterday-btn', 'today-btn', '24h-btn',
        'week-btn', 'month-btn', 'apply-btn'];

    presetIds.forEach(id =>
        {
            const btn = document.getElementById(id);
            if (id === activeId)
                btn.classList.add('active-preset');
            else
                btn.classList.remove('active-preset');
        }
    );
}

function applyFromInputs()
{
    const startStr = document.getElementById('start-date').value;
    const endStr = document.getElementById('end-date').value;
    if (!startStr || !endStr)
    {
        alert('Please select both start and end dates.');
        return;
    }

    const startTs = dateToUnixStart(startStr);
    const endTs = dateToUnixEnd(endStr);
    const label = `${startStr} – ${endStr}`;
    setActivePresetButton('apply-btn');
    updateChart(startTs, endTs, label);
}

function setDateInputs(startUnix, endUnix)
{
    const startDate = new Date(startUnix * 1000);
    const endDate = new Date(endUnix * 1000);
    document.getElementById('start-date').value = formatDate(startDate);
    document.getElementById('end-date').value = formatDate(endDate);
}

// =============================================================================


// =============================================================================
// Initialization.

document.addEventListener('DOMContentLoaded', async () =>
    {
        try
        {
            await fetchSensors();
        }
        catch(e)
        {
            console.error(e);
            alert('Could not load sensor list');
            return;
        }

        // Default: last 24 hours
        const nowMs = Date.now();
        const startMs = nowMs - 24 * 60 * 60 * 1000;
        setDateInputs(Math.floor(startMs / 1000), Math.floor(nowMs / 1000));
        setActivePresetButton('24h-btn');
        updateChart(Math.floor(startMs / 1000), Math.floor(nowMs / 1000), '24 hours');

        // Apply button
        document.getElementById('apply-btn').addEventListener('click', applyFromInputs);

        // Yesterday
        document.getElementById('yesterday-btn').addEventListener('click', () =>
            {
                const d = new Date();
                d.setDate(d.getDate() - 1);
                const yesterdayStr = formatDate(d);
                document.getElementById('start-date').value = yesterdayStr;
                document.getElementById('end-date').value = yesterdayStr;
                const startTs = dateToUnixStart(yesterdayStr);
                const endTs = dateToUnixEnd(yesterdayStr);
                setActivePresetButton('yesterday-btn');
                updateChart(startTs, endTs, 'Yesterday');
            }
        );

        // Today
        document.getElementById('today-btn').addEventListener('click', () =>
            {
                const today = getTodayStr();
                document.getElementById('start-date').value = today;
                document.getElementById('end-date').value = today;
                const startTs = dateToUnixStart(today);
                const endTs = dateToUnixEnd(today);
                setActivePresetButton('today-btn');
                updateChart(startTs, endTs, 'Today');
            }
        );

        // 24h
        document.getElementById('24h-btn').addEventListener('click', () =>
            {
                const nowMs = Date.now();
                const startMs = nowMs - 24 * 60 * 60 * 1000;
                setDateInputs(Math.floor(startMs / 1000), Math.floor(nowMs / 1000));
                setActivePresetButton('24h-btn');
                updateChart(Math.floor(startMs / 1000), Math.floor(nowMs / 1000), '24 hours');
            }
        );

        // Week
        document.getElementById('week-btn').addEventListener('click', () =>
            {
                const endDate = new Date();
                const startDate = new Date();
                startDate.setDate(startDate.getDate() - 6);
                document.getElementById('start-date').value = formatDate(startDate);
                document.getElementById('end-date').value = formatDate(endDate);
                const startTs = dateToUnixStart(formatDate(startDate));
                const endTs = dateToUnixEnd(formatDate(endDate));
                setActivePresetButton('week-btn');
                updateChart(startTs, endTs, 'Last week');
            }
        );

        // Month
        document.getElementById('month-btn').addEventListener('click', () =>
            {
                const endDate = new Date();
                const startDate = new Date();
                startDate.setDate(startDate.getDate() - 30);
                document.getElementById('start-date').value = formatDate(startDate);
                document.getElementById('end-date').value = formatDate(endDate);
                const startTs = dateToUnixStart(formatDate(startDate));
                const endTs = dateToUnixEnd(formatDate(endDate));
                setActivePresetButton('month-btn');
                updateChart(startTs, endTs, 'Last month');
            }
        );
    }
);

// =============================================================================
