// =============================================================================
//  Global variables.

// Array containing all sensors fetched from the API (both fixed and
// toggleable).
let allSensors = [];
// Subset of sensors with id < 100 that can be toggled on/off by the user.
let toggleableSensors = [];
// Set of sensor IDs (id >= 100) that are always displayed and cannot be
// toggled.
let fixedSensorIds = new Set();
// Set of toggleable sensor IDs currently enabled by the user via checkboxes.
let enabledToggleIds = new Set();

// Reference to the Chart.js instance (recreated on every rebuild).
let chart = null;
// Most recently fetched time-series data (used when toggling sensors to avoid
// re‑fetch).
let lastFetchedData = [];

// =============================================================================


// =============================================================================
// Date / Time Helpers.

/**
 * Converts a date string (YYYY-MM-DD) to a Unix timestamp representing the
 * start of that day (00:00:00 local time).
 * @param {string} dateStr - Date in YYYY-MM-DD format.
 * @returns {number} Unix timestamp in seconds.
 */
function dateToUnixStart(dateStr)
{
    const d = new Date(dateStr + 'T00:00:00');
    return Math.floor(d.getTime() / 1000);
}

/**
 * Converts a date string (YYYY-MM-DD) to a Unix timestamp representing the
 * end of that day (23:59:59 local time).
 * @param {string} dateStr - Date in YYYY-MM-DD format.
 * @returns {number} Unix timestamp in seconds.
 */
function dateToUnixEnd(dateStr)
{
    const d = new Date(dateStr + 'T23:59:59');
    return Math.floor(d.getTime() / 1000);
}

/**
 * Returns today's date as a string in YYYY-MM-DD format (local time).
 * @returns {string} Today's date.
 */
function getTodayStr()
{
    const now = new Date();
    const year = now.getFullYear();
    const month = String(now.getMonth() + 1).padStart(2, '0');
    const day = String(now.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
}

// =============================================================================


// =============================================================================
// API Calls.

/**
 * Fetches the list of all sensors from the backend.
 * Populates global allSensors, separates them into toggleable (id < 100) and
 * fixed (id >= 100) groups, and builds the checkbox UI.
 * @throws Will throw an error if the network request fails.
 */
async function fetchSensors()
{
    const resp = await fetch('/api/sensors');
    if (!resp.ok)
        throw new Error('Failed to fetch sensors');

    allSensors = await resp.json();
    // Sensors with id < 100 are user-toggleable.
    toggleableSensors = allSensors.filter(s => s.id < 100);
    // Sensors with id >= 100 are always shown.
    fixedSensorIds = new Set(allSensors.filter(s => s.id >= 100).map(s => s.id));
    // Start with no toggleable sensors enabled.
    enabledToggleIds = new Set();
    // Generate the checkboxes in the DOM.
    createToggleCheckboxes();
}

/**
 * Creates checkboxes for each toggleable sensor and attaches change listeners.
 * When a checkbox is toggled, the enabledToggleIds set is updated and the chart
 * is rebuilt using the already fetched data (lastFetchedData).
 */
function createToggleCheckboxes()
{
    const container = document.getElementById('toggleable-sensors');
    // Clear any existing checkboxes.
    container.innerHTML = '';
    toggleableSensors.forEach(sensor =>
        {
            const label = document.createElement('label');
            label.style.marginRight = '15px';
            const cb = document.createElement('input');
            cb.type = 'checkbox';
            cb.value = sensor.id;
            cb.checked = false;

            // Update the set of enabled IDs and rebuild chart on change.
            cb.addEventListener('change', () =>
                {
                    if (cb.checked)
                        enabledToggleIds.add(sensor.id);
                    else
                        enabledToggleIds.delete(sensor.id);
                    // No need to re‑fetch data.
                    rebuildChartWithCurrentData();
                }
            );
            label.appendChild(cb);
            label.appendChild(document.createTextNode(' ' + sensor.name));
            container.appendChild(label);
        }
    );
}

/**
 * Returns the complete set of sensor IDs that should currently be shown.
 * This includes all fixed sensors plus any toggleable sensors the user
 * has enabled.
 * @returns {Set<number>} Active sensor IDs.
 */
function getActiveSensorIds()
{
    const active = new Set([...fixedSensorIds]);
    for (let id of enabledToggleIds)
        active.add(id);
    return active;
}

/**
 * Fetches time-series data for a given Unix timestamp range (in seconds).
 * @param {number} startUnix - Start timestamp (seconds).
 * @param {number} endUnix - End timestamp (seconds).
 * @returns {Promise<Array>} The data array from the backend (objects with
 *                           sensor_id, date_time, value).
 */
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

/**
 * Transforms raw data points and sensor metadata into Chart.js dataset objects.
 * Only points belonging to activeSensorIds are included.
 * @param {Array} dataArray - Raw data from the API (each point has
 *                            sensor_id, date_time, value).
 * @param {Set<number>} activeSensorIds - Which sensors to display.
 * @param {Object} sensorMap - Mapping from sensor ID to sensor name.
 * @returns {Array<Object>} Array of dataset objects for Chart.js.
 */
function buildDatasets(dataArray, activeSensorIds, sensorMap)
{
    // Group points by sensor_id.
    const groups = {};
    dataArray.forEach(point =>
        {
            if (!activeSensorIds.has(point.sensor_id))
                return;

            const sid = point.sensor_id;
            if (!groups[sid])
                groups[sid] = [];
            // Convert timestamp to milliseconds for Chart.js time scale.
            groups[sid].push({ x: point.date_time * 1000, y: point.value });
        }
    );

    // Color palette for different sensor lines.
    const colors = [
        '#FF6384', '#36A2EB', '#FFCE56', '#4BC0C0', '#9966FF',
        '#FF9F40', '#E7E9ED', '#76B041', '#D19A66', '#9B59B6'
    ];

    const datasets = [];
    let colorIdx = 0;
    for (const [sid, points] of Object.entries(groups))
    {
        // Ensure points are sorted chronologically.
        points.sort((a, b) => a.x - b.x);
        const sensorName = sensorMap[sid] || `Sensor ${sid}`;

        // Calculate minimum value.
        const values = points.map(p => p.y);
        const minVal = Math.min(...values);
        const labelWithMin = `${sensorName} (${minVal.toFixed(1)})`;

        datasets.push(
            {
                label: labelWithMin,
                data: points,
                borderColor: colors[colorIdx % colors.length],
                backgroundColor: 'transparent', // No fill under the line
                pointRadius: 1,
                borderWidth: 2,
                tension: 0.1 // Slight curve to the line
            }
        );
        colorIdx++;
    }
    return datasets;
}

/**
 * Rebuilds the Chart.js line chart using the last fetched data and current
 * active sensor selection. Destroys any previous chart instance.
 */
function rebuildChartWithCurrentData()
{
    // If no data has been fetched yet, don't draw anything.
    if (!lastFetchedData.length)
        return;

    // Build a lookup of sensor id -> name from the full sensor list.
    const sensorMap = {};
    allSensors.forEach(s => { sensorMap[s.id] = s.name; });

    const activeIds = getActiveSensorIds();
    const datasets = buildDatasets(lastFetchedData, activeIds, sensorMap);

    const ctx = document.getElementById('sensorChart').getContext('2d');

    // Destroy previous chart instance to avoid canvas reuse issues.
    if (chart)
        chart.destroy();

    // Create new Chart.js line chart.
    chart = new Chart(ctx,
        {
            type: 'line',
            data:
                {
                    datasets // No labels; each dataset supplies its own x/y points.
                },
            options:
                {
                    responsive: true,
                    maintainAspectRatio: false, // Allows the chart to fill its container.
                    scales:
                        {
                            x:
                                {
                                    type: 'time',
                                    time:
                                        {
                                            unit: 'minute', // Default tick unit.
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
                                    mode: 'index', // Show all dataset values at a given x.
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

/**
 * Updates the text content of the "range-label" element to show the current
 * date range description.
 * @param {string} text - Label to display
 *                        (e.g., "24 hours", "2025-06-15 – 2025-06-16").
 */
function updateRangeLabel(text)
{
    document.getElementById('range-label').textContent = text;
}

/**
 * Fetches data for a new time range, updates lastFetchedData, rebuilds the
 * chart, and optionally sets the range label.
 * @param {number} startUnix - Start timestamp (seconds).
 * @param {number} endUnix - End timestamp (seconds).
 * @param {string} [labelText] - Optional label for the range display.
 */
async function updateChart(startUnix, endUnix, labelText)
{
    try
    {
        const data = await fetchData(startUnix, endUnix);
        // Store for later toggle changes.
        lastFetchedData = data;
        // Draw chart with active sensor set.
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

/**
 * Activates the preset button with the given ID by applying the 'active-preset'
 * CSS class, and removes that class from all other preset buttons.
 *
 * @param {string} activeId - The ID of the button that should become active
 *                            (e.g., 'today-btn').
 */
function setActivePresetButton(activeId)
{
    // All possible preset button IDs – make sure these match the actual DOM
    // elements.
    const presetIds = ['today-btn', '24h-btn', 'week-btn', 'month-btn', 'apply-btn'];
    presetIds.forEach(id =>
        {
            const btn = document.getElementById(id);
            // Toggle the active state based on whether this button is
            // the target.
            if (id === activeId)
                btn.classList.add('active-preset');
            else
                btn.classList.remove('active-preset');
        }
    );
}

// =============================================================================


// =============================================================================
// Event Handlers for Date Controls.

/**
 * Reads the manually entered start and end dates from the input fields,
 * converts them to Unix timestamps, and triggers a chart update.
 */
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

/**
 * Sets the start and end date input fields to the given Unix timestamps (seconds).
 * @param {number} startUnix - Start timestamp.
 * @param {number} endUnix - End timestamp.
 */
function setDateInputs(startUnix, endUnix)
{
    const startDate = new Date(startUnix * 1000);
    const endDate = new Date(endUnix * 1000);

    // Helper to format a Date object to YYYY-MM-DD.
    const fmt = (d) =>
        {
            const year = d.getFullYear();
            const month = String(d.getMonth() + 1).padStart(2, '0');
            const day = String(d.getDate()).padStart(2, '0');
            return `${year}-${month}-${day}`;
        };
    document.getElementById('start-date').value = fmt(startDate);
    document.getElementById('end-date').value = fmt(endDate);
}

// =============================================================================


// =============================================================================
// Initialization on Page Load.

document.addEventListener('DOMContentLoaded', async () =>
    {
        // 1. Fetch sensor metadata and build checkboxes.
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

        // 2. Default time range: last 24 hours.
        const nowMs = Date.now();
        const startMs = nowMs - 24 * 60 * 60 * 1000; // 24 hours ago in milliseconds.
        setDateInputs(Math.floor(startMs / 1000), Math.floor(nowMs / 1000));
        setActivePresetButton('24h-btn');
        updateChart(Math.floor(startMs / 1000), Math.floor(nowMs / 1000), '24 hours');

        // 3. Wire up the "Apply" button for custom date range.
        document.getElementById('apply-btn').addEventListener('click', applyFromInputs);

        // 4. Quick range buttons.

        // "Today" button: show data for the current calendar day.
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

        // "24h" button: last 24 hours.
        document.getElementById('24h-btn').addEventListener('click', () =>
            {
                const nowMs = Date.now();
                const startMs = nowMs - 24 * 60 * 60 * 1000;
                setDateInputs(Math.floor(startMs / 1000), Math.floor(nowMs / 1000));
                setActivePresetButton('24h-btn'); 
                updateChart(Math.floor(startMs / 1000), Math.floor(nowMs / 1000), '24 hours');
            }
        );

        // "Week" button: last 7 days.
        document.getElementById('week-btn').addEventListener('click', () =>
            {
                const endDate = new Date();
                const startDate = new Date();
                startDate.setDate(startDate.getDate() - 6);
                const fmt = (d) =>
                    {
                        const year = d.getFullYear();
                        const month = String(d.getMonth() + 1).padStart(2, '0');
                        const day = String(d.getDate()).padStart(2, '0');
                        return `${year}-${month}-${day}`;
                    };
                document.getElementById('start-date').value = fmt(startDate);
                document.getElementById('end-date').value = fmt(endDate);
                const startTs = dateToUnixStart(fmt(startDate));
                const endTs = dateToUnixEnd(fmt(endDate));
                setActivePresetButton('week-btn'); 
                updateChart(startTs, endTs, 'Last week');
            }
        );

        // "Month" button: last 30 days.
        document.getElementById('month-btn').addEventListener('click', () =>
            {
                const endDate = new Date();
                const startDate = new Date();
                startDate.setDate(startDate.getDate() - 30);
                const fmt = (d) =>
                    {
                        const year = d.getFullYear();
                        const month = String(d.getMonth() + 1).padStart(2, '0');
                        const day = String(d.getDate()).padStart(2, '0');
                        return `${year}-${month}-${day}`;
                    };
                document.getElementById('start-date').value = fmt(startDate);
                document.getElementById('end-date').value = fmt(endDate);
                const startTs = dateToUnixStart(fmt(startDate));
                const endTs = dateToUnixEnd(fmt(endDate));
                setActivePresetButton('month-btn');
                updateChart(startTs, endTs, 'Last month');
            }
        );
    }
);

// =============================================================================
