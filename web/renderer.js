function escapeHtml(text) {
    return text
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
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
        const errCls = block.isError ? 'error' : 'ok';
        return `<div class="content-block tool-result ${block.isError ? 'error' : ''}">
                    <div class="tool-result-label ${errCls}">${block.isError ? 'Error' : 'Result'}</div>
                    ${escapeHtml(block.text)}
                </div>`;
    }
    if (block.type === 'text') {
        return `<div class="content-block text-block">${escapeHtml(block.text)}</div>`;
    }
    return `<div class="content-block">${escapeHtml(block.text)}</div>`;
}

function renderEntry(entry) {
    const cls = entryClass(entry.type);
    const timestamp = entry.timestamp
        ? `<span class="timestamp">${escapeHtml(entry.timestamp)}</span>`
        : '';
    let html = `<div class="entry ${cls}">`;
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
    module.exports = { escapeHtml, entryClass, renderContentBlock, renderEntry, renderSession };
}
