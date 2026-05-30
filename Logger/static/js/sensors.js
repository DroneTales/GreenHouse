// =============================================================================
//  Global variables.

// When non-null, indicates we are editing an existing sensor (stores its ID).
// When null, the form is in "Add new sensor" mode.
let editingSensorId = null;

// =============================================================================


// =============================================================================
// Sensor List.

/**
 * Fetches the full list of sensors from the API and renders them into the
 * table with ID, name, and action buttons (Edit / Delete).
 * @throws Will throw if the network request fails.
 */
async function loadSensors()
{
    const resp = await fetch('/api/sensors');
    if (!resp.ok)
        throw new Error('Failed to load sensors');

    const sensors = await resp.json();
    const tbody = document.querySelector('#sensors-table tbody');
    // Clear any existing rows.
    tbody.innerHTML = '';

    sensors.forEach(sensor =>
        {
            const row = document.createElement('tr');
            // Build row HTML. Use escapeHtml() to prevent XSS attacks..
            row.innerHTML = `
                <td>${sensor.id}</td>
                <td>${escapeHtml(sensor.name)}</td>
                <td>
                    <button class="edit-btn" data-id="${sensor.id}" data-name="${escapeHtml(sensor.name)}">Edit</button>
                    <button class="delete-btn" data-id="${sensor.id}">Delete</button>
                </td>
            `;
            tbody.appendChild(row);
        }
    );

    // Attach click handlers to all Edit buttons.
    document.querySelectorAll('.edit-btn').forEach(btn =>
        {
            btn.addEventListener('click', () =>
                {
                    startEdit(btn.dataset.id, btn.dataset.name);
                }
            );
        }
    );

    // Attach click handlers to all Delete buttons.
    document.querySelectorAll('.delete-btn').forEach(btn =>
        {
            btn.addEventListener('click', () =>
                {
                    deleteSensor(btn.dataset.id);
                }
            );
        }
    );
}

// =============================================================================


// =============================================================================
// Utility: HTML Escaping.

/**
 * Safely escapes any HTML special characters in the provided text to prevent
 * cross-site scripting (XSS) when inserting into innerHTML.
 * @param {string} text - The untrusted text to escape.
 * @returns {string} The escaped string.
 */
function escapeHtml(text)
{
    const div = document.createElement('div');
    div.appendChild(document.createTextNode(text));
    return div.innerHTML;
}

// =============================================================================


// =============================================================================
// Form Mode Management (Add vs. Edit).

/**
 * Switches the form into "edit" mode for a specific sensor.
 * Pre-fills the form fields, makes the ID read-only, and shows the cancel button.
 * @param {string} id - The sensor ID being edited.
 * @param {string} name - The current sensor name.
 */
function startEdit(id, name)
{
    editingSensorId = id;
    document.getElementById('sensor-id').value = id;
    document.getElementById('sensor-name').value = name;
    // ID cannot be changed during edit.
    document.getElementById('sensor-id').readOnly = true;
    document.getElementById('form-title').textContent = 'Edit Sensor';
    document.getElementById('form-submit-btn').textContent = 'Update';
    document.getElementById('cancel-edit-btn').style.display = 'inline-block';
}

/**
 * Resets the form back to "add new sensor" mode.
 * Clears all fields, re-enables the ID input, and hides the cancel button.
 */
function cancelEdit()
{
    editingSensorId = null;
    document.getElementById('sensor-id').value = '';
    document.getElementById('sensor-name').value = '';
    // Allow entering a new ID.
    document.getElementById('sensor-id').readOnly = false;
    document.getElementById('form-title').textContent = 'Add New Sensor';
    document.getElementById('form-submit-btn').textContent = 'Add Sensor';
    document.getElementById('cancel-edit-btn').style.display = 'none';
}

// =============================================================================


// =============================================================================
// Form Submission (Create / Update).

/**
 * Handles the form submit event. Determines whether to create a new sensor or
 * update an existing one based on the editingSensorId global.
 * @param {Event} event - The form submit event.
 */
async function handleFormSubmit(event)
{
    event.preventDefault();
    const id = document.getElementById('sensor-id').value;
    const name = document.getElementById('sensor-name').value.trim();

    if (!id || !name)
    {
        alert('Both ID and name are required.');
        return;
    }

    // Determine URL and HTTP method based on whether we are editing.
    const url = editingSensorId ? `/api/sensors/${editingSensorId}` : '/api/sensors';
    const method = editingSensorId ? 'PUT' : 'POST';
    // For editing, only send the name; for creation, send both id and name.
    const payload = editingSensorId ? { name } : { id: parseInt(id), name };

    try
    {
        const resp = await fetch(url,
            {
                method,
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            }
        );
        const result = await resp.json();
        if (!resp.ok)
        {
            alert(result.error || 'Operation failed');
            return;
        }

        // Success: reset form and refresh the table.
        cancelEdit();
        loadSensors();
    }
    catch(err)
    {
        console.error(err);
        alert('Network error');
    }
}

// =============================================================================


// =============================================================================
// Delete Sensor.

/**
 * Sends a DELETE request for the given sensor ID after user confirmation.
 * @param {string} id - The ID of the sensor to delete.
 */

async function deleteSensor(id)
{
    if (!confirm(`Delete sensor ID ${id}?`))
        return;
    
    try
    {
        const resp = await fetch(`/api/sensors/${id}`, { method: 'DELETE' });
        const result = await resp.json();
        if (!resp.ok)
        {
            alert(result.error || 'Deletion failed');
            return;
        }

        // Refresh table to reflect the deletion.
        loadSensors();
    }
    catch(err)
    {
        console.error(err);
        alert('Network error');
    }
}

// =============================================================================


// =============================================================================
// Initialisation on Page Load.

document.addEventListener('DOMContentLoaded', () =>
    {
        // Bind form submit and cancel button events.
        document.getElementById('sensor-form').addEventListener('submit', handleFormSubmit);
        document.getElementById('cancel-edit-btn').addEventListener('click', cancelEdit);

        // Load the initial sensor list.
        loadSensors();
    }
);

// =============================================================================
