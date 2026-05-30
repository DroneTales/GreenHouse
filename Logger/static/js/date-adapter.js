// Minimal Chart.js time adapter using native Date
(
    function()
    {
        // =============================================================================
        // Helper: formatDate
        // Converts a date/time value (number, string, or Date) into a string
        // according to a simple format pattern. Supports only the placeholders
        // yyyy, MM, dd, HH, mm, ss.
        
        function formatDate(date, fmt)
        {
            const d = new Date(date);
            if (isNaN(d))
                // Invalid date.
                return '';
            
            const pad = (n) => ('0' + n).slice(-2);
            return fmt
                .replace('yyyy', d.getFullYear())
                .replace('MM', pad(d.getMonth() + 1))
                .replace('dd', pad(d.getDate()))
                .replace('HH', pad(d.getHours()))
                .replace('mm', pad(d.getMinutes()))
                .replace('ss', pad(d.getSeconds()));
        }

        // =============================================================================
    

        // =============================================================================
        // Helper: parse
        // Tries to parse a string according to a given format.
        // Default format is 'yyyy-MM-dd HH:mm:ss' which we also try to match via
        // a regex. Falls back to the built‑in Date.parse for other formats.
        // Returns a timestamp in milliseconds, or NaN on failure.

        function parse(str, fmt)
        {
            if (fmt === 'yyyy-MM-dd HH:mm:ss' || !fmt)
            {
                // Parse date and time parts using a regex.
                const parts = str.match(/(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})/);
                if (parts)
                {
                    // months are 0‑based in Date constructor.
                    return new Date(
                        +parts[1],
                        +parts[2] - 1,
                        +parts[3],
                        +parts[4],
                        +parts[5],
                        +parts[6]).getTime();
                }
            }
            // Fallback to native parser for all other formats.
            return Date.parse(str);
        }

        // =============================================================================


        // =============================================================================
        // Adapter object
        // Contains a single method: override, which injects the needed functions
        // into the Chart.js date adapter system.
        
        const adapter =
        {
            /**
             * Overrides Chart.js' internal date adapter methods.
             * @param {Object} chartjs - The global Chart object (or a reference to it).
             */
            override: function(chartjs)
                {
                    chartjs._adapters._date.override(
                        {
                            /**
                             * Returns the format strings for various standard time units.
                             * These are used for automatic tick label formatting.
                             */
                            formats: function()
                                {
                                    return {
                                        datetime: 'yyyy-MM-dd HH:mm:ss',
                                        millisecond: 'h:mm:ss.SSS a',
                                        second: 'h:mm:ss a',
                                        minute: 'h:mm a',
                                        hour: 'hA',
                                        day: 'MMM dd',
                                        week: 'MMM dd',
                                        month: 'MMM yyyy',
                                        quarter: "'Q'q - yyyy",
                                        year: 'yyyy'
                                    };
                                },

                            /**
                             * Parses a value into a timestamp (milliseconds).
                             * Handles Date objects, numbers (assumed to be milliseconds),
                             * and strings via the parse() helper.
                             * @param {Date|number|string} value - Input to parse.
                             * @param {string} fmt - Expected format string (used as hint).
                             * @returns {number|null} Timestamp in ms, or null if unparseable.
                             */
                            parse: function(value, fmt)
                                {
                                    if (value instanceof Date)
                                        return value.getTime();
                                    if (typeof value === 'number')
                                        return value;
                                    if (typeof value === 'string')
                                    {
                                        const ts = parse(value, fmt);
                                        if (!isNaN(ts))
                                            return ts;
                                    }
                                    return null;
                                },
                            
                            /**
                             * Formats a timestamp (milliseconds) into a string using the given format.
                             * @param {number} time - Timestamp in ms.
                             * @param {string} fmt - Format pattern (e.g., 'yyyy-MM-dd').
                             * @returns {string} Formatted date string.
                             */
                            format: function(time, fmt)
                                {
                                    return formatDate(new Date(time), fmt);
                                },
                            
                            /**
                             * Adds a given amount of a time unit to a timestamp.
                             * Returns the new timestamp in milliseconds.
                             * @param {number} time - Original timestamp.
                             * @param {number} amount - How many units to add (can be negative).
                             * @param {string} unit - One of 'millisecond', 'second', 'minute',
                             *                        'hour', 'day', 'week', 'month', 'quarter', 'year'.
                             */
                            add: function(time, amount, unit)
                                {
                                    const d = new Date(time);
                                    switch (unit)
                                    {
                                        case 'millisecond':
                                            d.setMilliseconds(d.getMilliseconds() + amount);
                                            break;
                                        case 'second':
                                            d.setSeconds(d.getSeconds() + amount);
                                            break;
                                        case 'minute':
                                            d.setMinutes(d.getMinutes() + amount);
                                            break;
                                        case 'hour':
                                            d.setHours(d.getHours() + amount);
                                            break;
                                        case 'day':
                                            d.setDate(d.getDate() + amount);
                                            break;
                                        case 'week':
                                            d.setDate(d.getDate() + amount * 7);
                                            break;
                                        case 'month':
                                            d.setMonth(d.getMonth() + amount);
                                            break;
                                        case 'quarter':
                                            d.setMonth(d.getMonth() + amount * 3);
                                            break;
                                        case 'year':
                                            d.setFullYear(d.getFullYear() + amount);
                                            break;
                                    }
                                    return d.getTime();
                                },
                            
                            /**
                             * Calculates the difference between two timestamps in a given unit.
                             * @param {number} max - The later timestamp (in ms).
                             * @param {number} min - The earlier timestamp (in ms).
                             * @param {string} unit - The unit to express the difference in.
                             * @returns {number} The difference, or NaN if unit is unknown.
                             */
                            diff: function(max, min, unit)
                                {
                                    const diffMs = max - min;
                                    const seconds = diffMs / 1000;
                                    const minutes = seconds / 60;
                                    const hours = minutes / 60;
                                    const days = hours / 24;
                                    
                                    switch (unit)
                                    {
                                        case 'millisecond':
                                            return diffMs;
                                        case 'second':
                                            return seconds;
                                        case 'minute':
                                            return minutes;
                                        case 'hour':
                                            return hours;
                                        case 'day':
                                            return days;
                                        case 'week':
                                            return days / 7;
                                        case 'month':
                                            // Approximate.
                                            return days / 30;
                                        case 'quarter':
                                            // Approximate.
                                            return days / 90;
                                        case 'year':
                                            // Approximate (no leap year handling).
                                            return days / 365;
                                    }
                                    return NaN;
                                },

                            /**
                             * Returns the start timestamp of the unit that contains the given time.
                             * For 'week', an additional 'weekday' argument (0=Sunday, 1=Monday, etc.)
                             * is used to align the start.
                             * @param {number} time - Timestamp in ms.
                             * @param {string} unit - The unit to snap to.
                             * @param {number} [weekday=0] - (Only for 'week') Desired first day of week.
                             * @returns {number} Timestamp of the unit start.
                             */
                            startOf: function(time, unit, weekday)
                                {
                                    const d = new Date(time);
                                    switch (unit)
                                    {
                                        case 'second':
                                            d.setMilliseconds(0);
                                            break;
                                        case 'minute':
                                            d.setSeconds(0, 0);
                                            break;
                                        case 'hour':
                                            d.setMinutes(0, 0, 0);
                                            break;
                                        case 'day':
                                            d.setHours(0, 0, 0, 0); break;
                                        case 'week':
                                            // Move to start of day, then adjust to the desired weekday.
                                            d.setHours(0, 0, 0, 0);
                                            const day = d.getDay();
                                            const diff = (day < weekday ? 7 : 0) + day - weekday;
                                            d.setDate(d.getDate() - diff);
                                            break;
                                        case 'month':
                                            d.setDate(1); d.setHours(0, 0, 0, 0);
                                            break;
                                        case 'quarter':
                                            // Determine quarter start month: 0, 3, 6, 9.
                                            const month = d.getMonth();
                                            const quarterStart = Math.floor(month / 3) * 3;
                                            d.setMonth(quarterStart, 1);
                                            d.setHours(0, 0, 0, 0);
                                            break;
                                        case 'year':
                                            d.setMonth(0, 1); d.setHours(0, 0, 0, 0);
                                            break;
                                    }
                                    return d.getTime();
                                },
                            
                            /**
                             * Returns the end timestamp of the unit that contains the given time.
                             * (e.g., end of day = 23:59:59.999)
                             * @param {number} time - Timestamp in ms.
                             * @param {string} unit - The unit to snap to.
                             * @returns {number} Timestamp of the unit end.
                             */
                            endOf: function(time, unit)
                                {
                                    const d = new Date(time);
                                    switch (unit)
                                    {
                                        case 'second':
                                            d.setMilliseconds(999);
                                            break;
                                        case 'minute':
                                            d.setSeconds(59, 999);
                                            break;
                                        case 'hour':
                                            d.setMinutes(59, 59, 999);
                                            break;
                                        case 'day':
                                            d.setHours(23, 59, 59, 999);
                                            break;
                                        case 'week':
                                            // Move to end of day, then advance to the end of the week (Saturday).
                                            d.setHours(23, 59, 59, 999);
                                            const day = d.getDay();
                                            d.setDate(d.getDate() + (6 - day));
                                            break;
                                        case 'month':
                                            // Set to the last day of the current month.
                                            d.setMonth(d.getMonth() + 1, 0);
                                            d.setHours(23, 59, 59, 999);
                                            break;
                                        case 'quarter':
                                            // Determine quarter end month: 2,5,8,11, then set to last day of that month.
                                            const month = d.getMonth();
                                            const quarterEnd = Math.floor(month / 3) * 3 + 2;
                                            d.setMonth(quarterEnd + 1, 0);
                                            d.setHours(23, 59, 59, 999);
                                            break;
                                        case 'year':
                                            d.setMonth(11, 31);
                                            d.setHours(23, 59, 59, 999);
                                            break;
                                    }
                                    return d.getTime();
                                }
                        }
                    );
                }
        };

        // =============================================================================


        // =============================================================================
        // Auto‑registration
        // If Chart is already defined (script loaded after Chart.js), override
        // immediately. Otherwise, wait for the window 'load' event, at which point
        // Chart.js should be available.
        
        if (typeof Chart !== 'undefined')
            adapter.override(Chart);
        else
        {
            window.addEventListener('load', function()
                {
                    adapter.override(Chart);
                }
            );
        }

        // =============================================================================
    }
)();
