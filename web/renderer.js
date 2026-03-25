function escapeHtml(text) {
    return text
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

function formatTimestamp(iso) {
    const date = new Date(iso);
    if (isNaN(date)) return iso;
    const hh = String(date.getUTCHours()).padStart(2, '0');
    const mm = String(date.getUTCMinutes()).padStart(2, '0');
    const ss = String(date.getUTCSeconds()).padStart(2, '0');
    return `${hh}:${mm}:${ss}`;
}

function entryClass(type) {
    if (type === 'user' || type === 'assistant') return type;
    return 'metadata';
}

function renderContentBlock(block) {
    if (block.type === 'thinking') {
        return `<div class="content-block thinking">
                    <div class="thinking-label">Thinking</div>
                    <div class="thinking-content">${escapeHtml(block.text)}</div>
                </div>`;
    }
    if (block.type === 'tool_use') {
        let inputText = block.toolInput;
        try { inputText = JSON.stringify(JSON.parse(block.toolInput), null, 2); } catch(e) {}
        return `<div class="content-block tool-use">
                    <div class="tool-use-label">${escapeHtml(block.toolName)}</div>
                    <div class="tool-use-input">${escapeHtml(inputText)}</div>
                </div>`;
    }
    if (block.type === 'tool_result') {
        const label = block.isError ? 'Error' : 'Result';
        const errorClass = block.isError ? ' error' : '';
        return `<div class="content-block tool-result${errorClass}">
                    <div class="tool-result-label${errorClass}">${label}</div>
                    ${escapeHtml(block.text)}
                </div>`;
    }
    const blockClass = block.type === 'text' ? ' text-block' : '';
    return `<div class="content-block${blockClass}">${escapeHtml(block.text)}</div>`;
}

function renderEntry(entry) {
    const cls = entryClass(entry.type);
    const hidden = cls === 'metadata' ? ' hidden' : '';
    const timestamp = entry.timestamp
        ? `<span class="timestamp">${formatTimestamp(entry.timestamp)}</span>`
        : '';
    let html = `<div class="entry ${cls}${hidden}">`;
    html += `<div class="entry-header"><span>${escapeHtml(entry.type)}</span>${timestamp}</div>`;

    if (entry.content.size() > 0) {
        html += '<div class="entry-body">';
        for (let i = 0; i < entry.content.size(); i++) {
            html += renderContentBlock(entry.content.get(i));
        }
        html += '</div>';
    }

    html += '</div>';
    return html;
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

    for (let i = 0; i < session.entries.size(); i++) {
        const entry = session.entries.get(i);
        if (entry.type === 'ai-title') continue;
        html += renderEntry(entry);
    }

    return html;
}

if (typeof module !== 'undefined') {
    module.exports = { escapeHtml, formatTimestamp, entryClass, renderContentBlock, renderEntry, renderSession };
}
