const assert = require('assert');
global.marked = { parse: text => text.replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>') };
const { renderEntry, renderContentBlock, renderSession, entryClass, escapeHtml } = require('../web/renderer.js');

function vec(arr) {
    return { size: () => arr.length, get: i => arr[i] };
}

function makeBlock(overrides = {}) {
    return { type: 'text', text: '', toolName: '', toolInput: '', toolUseId: '', isError: false, ...overrides };
}

function makeEntry(overrides = {}) {
    return { type: 'user', timestamp: '', content: vec([]), ...overrides };
}

function makeSession(overrides = {}) {
    return { title: '', errors: vec([]), entries: vec([]), ...overrides };
}

function test(name, fn) {
    try {
        fn();
        console.log(`  PASS: ${name}`);
    } catch (e) {
        console.log(`  FAIL: ${name}`);
        console.log(`    ${e.message}`);
        process.exitCode = 1;
    }
}

console.log('renderer tests:');

test('renderEntry renders user message with text content', () => {
    const entry = makeEntry({
        timestamp: '2026-03-25T06:20:15.000Z',
        content: vec([makeBlock({ text: 'hello world' })])
    });

    const html = renderEntry(entry);

    assert(html.includes('user'), 'should contain entry type');
    assert(html.includes('hello world'), 'should contain text');
    assert(html.includes('06:20:15'), 'should contain formatted timestamp');
    assert(html.includes('class="entry user"'), 'should have user class');
});

test('renderContentBlock renders thinking block', () => {
    const html = renderContentBlock(makeBlock({ type: 'thinking', text: 'let me think...' }));

    assert(html.includes('thinking-label'), 'should have thinking label');
    assert(html.includes('let me think...'), 'should contain thinking text');
});

test('renderContentBlock renders tool_use block', () => {
    const html = renderContentBlock(makeBlock({ type: 'tool_use', toolName: 'Bash', toolInput: '{"command":"ls"}', toolUseId: 'toolu_123' }));

    assert(html.includes('Bash'), 'should contain tool name');
    assert(html.includes('ls'), 'should contain command');
});

test('renderContentBlock renders tool_result with error', () => {
    const html = renderContentBlock(makeBlock({ type: 'tool_result', text: 'command not found', isError: true }));

    assert(html.includes('Error'), 'should show Error label');
    assert(html.includes('command not found'), 'should contain error text');
    assert(html.includes('error'), 'should have error class');
});

test('renderSession renders title and entries', () => {
    const session = makeSession({
        title: 'My Session',
        entries: vec([
            makeEntry({ content: vec([makeBlock({ text: 'hi' })]) }),
            makeEntry({ type: 'assistant', content: vec([makeBlock({ text: 'hello' })]) })
        ])
    });

    const html = renderSession(session);

    assert(html.includes('My Session'), 'should contain title');
    assert(html.includes('hi'), 'should contain user text');
    assert(html.includes('hello'), 'should contain assistant text');
});

test('renderSession renders parse errors', () => {
    const session = makeSession({
        errors: vec([{ lineNumber: 3, rawLine: 'bad json' }])
    });

    const html = renderSession(session);

    assert(html.includes('1 parse error'), 'should show error count');
    assert(html.includes('Line 3'), 'should show line number');
    assert(html.includes('bad json'), 'should show raw line');
});

test('renderEntry hides metadata entries by default', () => {
    const html = renderEntry(makeEntry({ type: 'queue-operation', timestamp: '2026-03-25T06:20:14.840Z' }));

    assert(html.includes('hidden'), 'metadata entry should have hidden style');
});

test('renderEntry formats timestamp as human-readable time', () => {
    const entry = makeEntry({
        timestamp: '2026-03-25T06:20:15.000Z',
        content: vec([makeBlock({ text: 'hi' })])
    });

    const html = renderEntry(entry);

    assert(!html.includes('2026-03-25T06:20:15.000Z'), 'should not show raw ISO timestamp');
    assert(html.includes('06:20:15'), 'should show formatted time');
});

test('entryClass returns metadata for non-message types', () => {
    assert.strictEqual(entryClass('user'), 'user');
    assert.strictEqual(entryClass('assistant'), 'assistant');
    assert.strictEqual(entryClass('queue-operation'), 'metadata');
    assert.strictEqual(entryClass('file-history-snapshot'), 'metadata');
});

test('renderContentBlock shows tool description, hides input and result', () => {
    const block = makeBlock({ type: 'tool_use', toolName: 'Bash', toolInput: '{"command":"ls","description":"List files"}', toolUseId: 'toolu_123' });
    const toolIndex = { getResult: id => id === 'toolu_123' ? makeBlock({ type: 'tool_result', text: 'file1.txt', toolUseId: 'toolu_123' }) : undefined };

    const html = renderContentBlock(block, toolIndex);

    assert(html.includes('List files'), 'should show description');
    assert(html.includes('tool-use-details'), 'should have collapsible details wrapper');
});

test('renderContentBlock renders Bash command in a copyable code element', () => {
    const block = makeBlock({ type: 'tool_use', toolName: 'Bash', toolInput: '{"command":"ls -la","description":"List files"}' });

    const html = renderContentBlock(block, { getResult: () => undefined });

    assert(html.includes('<code'), 'should render command in a code element');
    assert(html.includes('ls -la'), 'should contain the command');
});

test('renderSession groups tool_result under matching tool_use', () => {
    const session = makeSession({
        entries: vec([
            makeEntry({ type: 'assistant', content: vec([
                makeBlock({ type: 'tool_use', toolName: 'Bash', toolInput: '{"command":"ls"}', toolUseId: 'toolu_123' })
            ])}),
            makeEntry({ content: vec([
                makeBlock({ type: 'tool_result', text: 'file1.txt\nfile2.txt', toolUseId: 'toolu_123' })
            ])})
        ])
    });

    const html = renderSession(session);

    assert(html.includes('Bash'), 'should show tool name');
    assert(html.includes('file1.txt'), 'should show tool result');
    assert(!html.includes('class="entry user"'), 'should not render user entry for tool_result-only messages');
});

test('renderSession keeps user entry when it has non-tool_result content', () => {
    const session = makeSession({
        entries: vec([
            makeEntry({ content: vec([
                makeBlock({ text: 'hello' }),
                makeBlock({ type: 'tool_result', text: 'output', toolUseId: 'toolu_456' })
            ])})
        ])
    });

    const html = renderSession(session);

    assert(html.includes('class="entry user"'), 'should keep user entry');
    assert(html.includes('hello'), 'should show text content');
});

test('escapeHtml escapes special characters', () => {
    assert.strictEqual(escapeHtml('<script>alert("xss")</script>'), '&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;');
});

test('renderContentBlock renders markdown in text blocks', () => {
    const html = renderContentBlock(makeBlock({ text: 'hello **world**' }));

    assert(html.includes('<strong>world</strong>'), 'should render bold markdown as <strong>');
});

test('renderToolResultBlock applies ok class to non-error results', () => {
    const html = renderContentBlock(makeBlock({ type: 'tool_result', text: 'success' }));

    assert(html.includes('tool-result-label ok'), 'should have ok class on label');
});

test('renderMarkdown falls back to escapeHtml when marked is unavailable', () => {
    const savedMarked = global.marked;
    delete global.marked;
    // Re-require to pick up absence — but renderMarkdown checks at call time
    const { renderMarkdown } = require('../web/renderer.js');

    const html = renderMarkdown('<b>bold</b>');

    assert.strictEqual(html, '&lt;b&gt;bold&lt;/b&gt;');
    global.marked = savedMarked;
});

test('renderToolUseBlock handles malformed toolInput gracefully', () => {
    const block = makeBlock({ type: 'tool_use', toolName: 'Bash', toolInput: 'not json' });

    const html = renderContentBlock(block);

    assert(html.includes('Bash'), 'should still show tool name');
    assert(html.includes('not json'), 'should show raw input as fallback');
});

test('renderEntry renders entry with zero content blocks', () => {
    const entry = makeEntry({ type: 'assistant' });

    const html = renderEntry(entry);

    assert(html.includes('class="entry assistant"'), 'should have assistant class');
    assert(!html.includes('entry-body'), 'should not have entry body');
});

test('renderEntry renders entry without timestamp', () => {
    const entry = makeEntry({ content: vec([makeBlock({ text: 'hi' })]) });

    const html = renderEntry(entry);

    assert(!html.includes('timestamp'), 'should not have timestamp span');
    assert(html.includes('hi'), 'should still show content');
});

test('formatTimestamp returns raw string for invalid date', () => {
    const { formatTimestamp } = require('../web/renderer.js');

    assert.strictEqual(formatTimestamp('not-a-date'), 'not-a-date');
});

test('renderMarkdown uses DOMPurify when available', () => {
    global.DOMPurify = { sanitize: html => html.replace(/<script.*?<\/script>/g, '') };
    const { renderMarkdown } = require('../web/renderer.js');

    const html = renderMarkdown('hello **world**');

    assert(html.includes('<strong>world</strong>'), 'should still render markdown');
    delete global.DOMPurify;
});

test('renderEntry classifies unknown type as metadata', () => {
    const entry = makeEntry({ type: 'unknown', content: vec([makeBlock({ text: 'data' })]) });

    const html = renderEntry(entry);

    assert(html.includes('metadata'), 'should have metadata class');
    assert(html.includes('hidden'), 'should be hidden by default');
});

test('renderToolUseBlock with empty toolInput shows empty details', () => {
    const block = makeBlock({ type: 'tool_use', toolName: 'Read', toolInput: '' });

    const html = renderContentBlock(block);

    assert(html.includes('Read'), 'should show tool name');
    assert(html.includes('tool-use-details'), 'should have details section');
});

test('renderSession with only tool_result entries renders nothing', () => {
    const session = makeSession({
        entries: vec([
            makeEntry({ content: vec([
                makeBlock({ type: 'tool_result', text: 'output', toolUseId: 'toolu_999' })
            ])})
        ])
    });

    const html = renderSession(session);

    assert(!html.includes('entry'), 'should not render any entries');
});

test('indexToolResults does not mark mixed content entry as result-only', () => {
    const session = makeSession({
        entries: vec([
            makeEntry({ content: vec([
                makeBlock({ text: 'hello' }),
                makeBlock({ type: 'tool_result', text: 'output', toolUseId: 'toolu_789' })
            ])})
        ])
    });

    const html = renderSession(session);

    assert(html.includes('class="entry user"'), 'should keep entry with mixed content');
    assert(html.includes('hello'), 'should show text content');
});
