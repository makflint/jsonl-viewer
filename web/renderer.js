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

function renderToolUseBlock(block, toolResults) {
    let inputObj = {};
    let inputText = block.toolInput;
    try {
        inputObj = JSON.parse(block.toolInput);
        inputText = JSON.stringify(inputObj, null, 2);
    } catch(e) {}
    const description = inputObj.description || '';
    const command = inputObj.command || '';
    const result = toolResults && toolResults[block.toolUseId];
    const resultHtml = result ? renderToolResultBlock(result) : '';
    const descHtml = description ? `<div class="tool-use-description">${escapeHtml(description)}</div>` : '';
    const commandHtml = command ? `<code class="tool-use-command">${escapeHtml(command)}</code>` : '';
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

function renderContentBlock(block, toolResults) {
    if (block.type === 'thinking') return renderThinkingBlock(block);
    if (block.type === 'tool_use') return renderToolUseBlock(block, toolResults);
    if (block.type === 'tool_result') return renderToolResultBlock(block);
    return renderTextBlock(block);
}

function renderEntry(entry, toolResults) {
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
            html += renderContentBlock(entry.content.get(i), toolResults);
        }
        html += '</div>';
    }

    html += '</div>';
    return html;
}

function buildToolResultMap(entries) {
    const map = {};
    for (let i = 0; i < entries.size(); i++) {
        const entry = entries.get(i);
        for (let j = 0; j < entry.content.size(); j++) {
            const block = entry.content.get(j);
            if (block.type === 'tool_result' && block.toolUseId) {
                map[block.toolUseId] = block;
            }
        }
    }
    return map;
}

function isToolResultOnly(entry) {
    if (entry.content.size() === 0) return false;
    for (let i = 0; i < entry.content.size(); i++) {
        if (entry.content.get(i).type !== 'tool_result') return false;
    }
    return true;
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

    const toolResults = buildToolResultMap(session.entries);

    for (let i = 0; i < session.entries.size(); i++) {
        const entry = session.entries.get(i);
        if (isToolResultOnly(entry)) continue;
        html += renderEntry(entry, toolResults);
    }

    return html;
}

if (typeof module !== 'undefined') {
    module.exports = { escapeHtml, formatTimestamp, entryClass, renderMarkdown, renderContentBlock, renderEntry, renderSession };
}
