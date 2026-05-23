function escapeHtml(text) {
    return text
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

function vecLen(v) { return v.size ? v.size() : v.length; }
function vecGet(v, i) { return v.get ? v.get(i) : v[i]; }

function formatTimestamp(iso) {
    const date = new Date(iso);
    if (isNaN(date)) return iso;
    const hh = String(date.getUTCHours()).padStart(2, '0');
    const mm = String(date.getUTCMinutes()).padStart(2, '0');
    const ss = String(date.getUTCSeconds()).padStart(2, '0');
    return `${hh}:${mm}:${ss}`;
}

function entryClass(type) {
    if (type === 'user' || type === 'assistant' || type === 'raw') return type;
    return 'metadata';
}

function highlightCode(code, language) {
    if (typeof hljs !== 'undefined') return hljs.highlight(code, { language }).value;
    return escapeHtml(code);
}

function highlightCommand(command) {
    if (typeof hljs === 'undefined') return escapeHtml(command);

    var inlineMatch = command.match(/^(.*?python3?\s+-c\s+)(\\?["'])([\s\S]*)\2([\s\S]*)$/);
    if (inlineMatch) {
        return highlightCode(inlineMatch[1], 'bash')
            + escapeHtml(inlineMatch[2])
            + highlightCode(inlineMatch[3], 'python')
            + escapeHtml(inlineMatch[2])
            + highlightCode(inlineMatch[4], 'bash');
    }

    var heredocMatch = command.match(/^(.*?python3?\s*<<\s*'?(\w+)'?\n)([\s\S]*?)\n\2(.*)$/);
    if (heredocMatch) {
        return highlightCode(heredocMatch[1], 'bash')
            + highlightCode(heredocMatch[3], 'python')
            + '\n' + heredocMatch[2]
            + highlightCode(heredocMatch[4], 'bash');
    }

    return highlightCode(command, 'bash');
}

function renderToolUseBlock(block, toolIndex) {
    let inputObj = {};
    let inputText = block.toolInput;
    try {
        inputObj = JSON.parse(block.toolInput);
        inputText = JSON.stringify(inputObj, null, 2);
    } catch(e) {}
    const description = inputObj.description || '';
    const command = inputObj.command || '';
    const result = toolIndex && toolIndex.getResult(block.toolUseId);
    const resultHtml = result ? renderToolResultBlock(result) : '';
    const descHtml = description ? `<div class="tool-use-description">${escapeHtml(description)}</div>` : '';
    const commandHtml = command ? `<code class="tool-use-command">${highlightCommand(command)}</code>` : '';
    return `<div class="content-block tool-use">
                <div class="tool-use-label">${escapeHtml(block.toolName)}</div>
                ${descHtml}
                ${commandHtml}
                <div class="tool-use-details">
                    <div class="tool-use-input">${escapeHtml(inputText)}</div>
                    ${resultHtml}
                </div>
            </div>`;
}

function renderToolResultBlock(block) {
    const label = block.isError ? 'Error' : 'Result';
    const statusClass = block.isError ? ' error' : '';
    const labelClass = block.isError ? ' error' : ' ok';
    return `<div class="content-block tool-result${statusClass}"><div class="tool-result-label${labelClass}">${label}</div><div class="tool-result-text">${escapeHtml(block.text)}</div></div>`;
}

function renderThinkingBlock(block) {
    return `<div class="content-block thinking">
                <div class="thinking-label">Thinking</div>
                <div class="thinking-content">${escapeHtml(block.text)}</div>
            </div>`;
}

function renderMarkdown(text) {
    if (typeof marked !== 'undefined') {
        const html = marked.parse(text);
        if (typeof DOMPurify !== 'undefined') return DOMPurify.sanitize(html);
        return html;
    }
    return escapeHtml(text);
}

function renderTextBlock(block) {
    const blockClass = block.type === 'text' ? ' text-block' : '';
    return `<div class="content-block${blockClass}">${renderMarkdown(block.text)}</div>`;
}

function renderRawBlock(block) {
    return `<div class="content-block raw"><pre><code class="hljs-json">${highlightCode(block.text, 'json')}</code></pre></div>`;
}

function renderContentBlock(block, toolIndex) {
    if (block.type === 'thinking') return renderThinkingBlock(block);
    if (block.type === 'tool_use') return renderToolUseBlock(block, toolIndex);
    if (block.type === 'tool_result') return renderToolResultBlock(block);
    if (block.type === 'raw') return renderRawBlock(block);
    return renderTextBlock(block);
}

function renderEntry(entry, toolIndex) {
    const cls = entryClass(entry.type);
    const hidden = cls === 'metadata' ? ' hidden' : '';
    const meta = entry.timestamp
        ? `<span class="timestamp">${formatTimestamp(entry.timestamp)}</span>`
        : entry.lineNumber > 0
            ? `<span class="timestamp">line ${entry.lineNumber}</span>`
            : '';
    let html = `<div class="entry ${cls}${hidden}">`;
    html += `<div class="entry-header"><span>${escapeHtml(entry.type)}</span>${meta}</div>`;

    if (entry.content.size() > 0) {
        html += '<div class="entry-body">';
        for (let i = 0; i < entry.content.size(); i++) {
            html += renderContentBlock(entry.content.get(i), toolIndex);
        }
        html += '</div>';
    }

    html += '</div>';
    return html;
}

function indexToolResults(entries) {
    const resultMap = {};
    const resultOnlyIndices = new Set();

    for (let i = 0; i < entries.size(); i++) {
        const entry = entries.get(i);
        let allToolResults = entry.content.size() > 0;
        for (let j = 0; j < entry.content.size(); j++) {
            const block = entry.content.get(j);
            if (block.type === 'tool_result') {
                if (block.toolUseId) resultMap[block.toolUseId] = block;
            } else {
                allToolResults = false;
            }
        }
        if (allToolResults) resultOnlyIndices.add(i);
    }

    return {
        getResult(toolUseId) { return resultMap[toolUseId]; },
        isResultOnlyEntry(index) { return resultOnlyIndices.has(index); }
    };
}

function renderSession(session) {
    let html = '';

    if (session.title) {
        html += `<div class="session-title">${escapeHtml(session.title)}</div>`;
    }

    if (session.errors.size() > 0) {
        html += '<div class="errors">';
        html += `<strong>${session.errors.size()} parse error(s):</strong><br>`;
        for (let i = 0; i < session.errors.size(); i++) {
            const err = session.errors.get(i);
            html += `Line ${err.lineNumber}: ${escapeHtml(err.rawLine)}<br>`;
        }
        html += '</div>';
    }

    const toolIndex = indexToolResults(session.entries);

    for (let i = 0; i < session.entries.size(); i++) {
        // Tool results are already nested inside their matching tool_use block,
        // so skip entries that contain only tool_result blocks
        if (toolIndex.isResultOnlyEntry(i)) continue;
        html += renderEntry(session.entries.get(i), toolIndex);
    }

    return html;
}

function isJsonl(text) {
    const firstLine = text.split('\n').find(l => l.trim().length > 0);
    if (!firstLine) return false;
    try { JSON.parse(firstLine); return true; } catch(e) { return false; }
}

function columnDepth(col) {
    const len = vecLen(col.children);
    if (col.kind !== 'object' || len === 0) return 1;
    let maxChild = 0;
    for (let i = 0; i < len; i++) {
        const d = columnDepth(vecGet(col.children, i));
        if (d > maxChild) maxChild = d;
    }
    return 1 + maxChild;
}

function leafCount(col) {
    const len = vecLen(col.children);
    if (col.kind !== 'object' || len === 0) return 1;
    let total = 0;
    for (let i = 0; i < len; i++) {
        total += leafCount(vecGet(col.children, i));
    }
    return total;
}

function buildHeaderRows(columns) {
    let maxDepth = 1;
    for (const c of columns) {
        const d = columnDepth(c);
        if (d > maxDepth) maxDepth = d;
    }

    const rows = Array.from({length: maxDepth}, () => []);

    function place(col, depth) {
        if (col.kind === 'object') {
            rows[depth].push({
                name: col.name,
                path: col.path,
                kind: col.kind,
                colspan: leafCount(col),
                rowspan: 1
            });
            const len = vecLen(col.children);
            for (let i = 0; i < len; i++) {
                place(vecGet(col.children, i), depth + 1);
            }
        } else {
            rows[depth].push({
                name: col.name,
                path: col.path,
                kind: col.kind,
                colspan: 1,
                rowspan: maxDepth - depth
            });
        }
    }

    for (const c of columns) place(c, 0);
    return rows;
}

function extractCellValue(obj, path) {
    const parts = path.split('.');
    let cur = obj;
    for (const part of parts) {
        if (cur === null || cur === undefined || typeof cur !== 'object') return undefined;
        cur = cur[part];
    }
    return cur;
}

function summarizeArrayCell(arr) {
    if (!Array.isArray(arr) || arr.length === 0) return "";
    const parts = arr.map(item => {
        if (item === null || item === undefined) return "";
        if (typeof item !== 'object') return String(item);
        for (const key of Object.keys(item)) {
            const v = item[key];
            if (typeof v === 'string') return v;
        }
        return "";
    }).filter(s => s !== "");
    const joined = parts.join(", ");
    if (joined.length <= 200) return joined;
    return joined.slice(0, 200) + "...";
}

function collectLeafColumns(columns) {
    const leaves = [];
    function walk(col) {
        if (col.kind === 'object') {
            const len = vecLen(col.children);
            for (let i = 0; i < len; i++) walk(vecGet(col.children, i));
        } else {
            leaves.push(col);
        }
    }
    const len = vecLen(columns);
    for (let i = 0; i < len; i++) walk(vecGet(columns, i));
    return leaves;
}

function formatCell(value, kind) {
    if (value === null || value === undefined) return '<span class="cell-null">—</span>';
    if (kind === 'array_summary') {
        const summary = summarizeArrayCell(value);
        return summary === "" ? '<span class="cell-null">—</span>' : escapeHtml(summary);
    }
    return escapeHtml(String(value));
}

function renderRawTable(rawEntries, schema) {
    const topCols = [];
    const len = vecLen(schema.columns);
    for (let i = 0; i < len; i++) topCols.push(vecGet(schema.columns, i));

    const headerRows = buildHeaderRows(topCols);
    const leafCols = collectLeafColumns(schema.columns);

    let html = '<table class="raw-table"><thead>';
    for (const row of headerRows) {
        html += '<tr>';
        for (const cell of row) {
            const attrs = ` data-path="${escapeHtml(cell.path)}"`
                       + (cell.colspan > 1 ? ` colspan="${cell.colspan}"` : '')
                       + (cell.rowspan > 1 ? ` rowspan="${cell.rowspan}"` : '');
            html += `<th${attrs}>${escapeHtml(cell.name)}</th>`;
        }
        html += '</tr>';
    }
    html += '</thead><tbody>';
    for (const entry of rawEntries) {
        html += '<tr>';
        for (const col of leafCols) {
            const value = extractCellValue(entry, col.path);
            html += `<td data-path="${escapeHtml(col.path)}">${formatCell(value, col.kind)}</td>`;
        }
        html += '</tr>';
    }
    html += '</tbody></table>';
    return html;
}

function statsForLeaf(stats) {
    const parts = [];
    // Type breakdown
    const typeStrs = [];
    if (stats.typeCounts) {
        for (const k in stats.typeCounts) typeStrs.push(`${k}: ${stats.typeCounts[k]}`);
    }
    if (typeStrs.length) parts.push(typeStrs.join(', '));
    // Numeric range
    const hasMin = stats.hasNumericMin !== undefined ? stats.hasNumericMin : stats.numericMin !== undefined;
    if (hasMin) parts.push(`range: ${stats.numericMin} .. ${stats.numericMax}`);
    // Top values
    if (stats.topValues) {
        const len = vecLen(stats.topValues);
        if (len > 0) {
            const top = [];
            for (let i = 0; i < len; i++) {
                const tv = vecGet(stats.topValues, i);
                top.push(`${escapeHtml(tv.value)} (${tv.count})`);
            }
            parts.push(`top: ${top.join(', ')}`);
        }
    }
    // Array stats
    const hasArrAvg = stats.hasArrayAvgLength !== undefined ? stats.hasArrayAvgLength : stats.arrayAvgLength !== undefined;
    if (hasArrAvg) parts.push(`avg length: ${stats.arrayAvgLength.toFixed(1)}, max: ${stats.arrayMaxLength}`);
    return parts.join('<br>');
}

function renderStatsRow(col, recordCount, depth) {
    const indent = '&nbsp;'.repeat(depth * 2);
    const presence = `${col.stats.presentCount || 0}/${recordCount}`;
    const pct = recordCount > 0 ? Math.round(100 * (col.stats.presentCount || 0) / recordCount) : 0;
    const nullPart = col.stats.nullCount ? ` &nbsp; null: ${col.stats.nullCount}` : '';
    let html = `<tr>
        <td class="stats-name">${indent}${escapeHtml(col.path)}</td>
        <td class="stats-kind">${escapeHtml(col.kind)}</td>
        <td class="stats-presence">${presence} (${pct}%)${nullPart}</td>
        <td class="stats-detail">${statsForLeaf(col.stats)}</td>
    </tr>`;
    if (col.kind === 'object') {
        const len = vecLen(col.children);
        for (let i = 0; i < len; i++) {
            html += renderStatsRow(vecGet(col.children, i), recordCount, depth + 1);
        }
    }
    return html;
}

function renderSchemaStats(schema) {
    let html = `<div class="schema-stats">
        <div class="schema-stats-header">
            <h3>Schema statistics (${schema.recordCount} records)</h3>
            <a href="#raw-table" class="jump-link">↑ jump to table</a>
        </div>
        <table class="stats-table"><tbody>`;
    const len = vecLen(schema.columns);
    for (let i = 0; i < len; i++) {
        html += renderStatsRow(vecGet(schema.columns, i), schema.recordCount, 0);
    }
    html += '</tbody></table></div>';
    return html;
}

if (typeof module !== 'undefined') {
    module.exports = { escapeHtml, formatTimestamp, entryClass, renderMarkdown, isJsonl, renderContentBlock, renderEntry, renderSession, extractCellValue, summarizeArrayCell, buildHeaderRows, renderRawTable, renderSchemaStats };
}
