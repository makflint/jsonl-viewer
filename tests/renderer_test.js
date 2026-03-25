const assert = require('assert');
const { renderEntry, renderContentBlock, renderSession, entryClass, escapeHtml } = require('../web/renderer.js');

function vec(arr) {
    return { size: () => arr.length, get: i => arr[i] };
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
    const entry = {
        type: 'user',
        timestamp: '2026-03-25T06:20:15.000Z',
        content: vec([
            { type: 'text', text: 'hello world', toolName: '', toolInput: '', toolUseId: '', isError: false }
        ])
    };

    const html = renderEntry(entry);

    assert(html.includes('user'), 'should contain entry type');
    assert(html.includes('hello world'), 'should contain text');
    assert(html.includes('2026-03-25'), 'should contain timestamp');
    assert(html.includes('class="entry user"'), 'should have user class');
});

test('renderContentBlock renders thinking block', () => {
    const block = { type: 'thinking', text: 'let me think...', toolName: '', toolInput: '', toolUseId: '', isError: false };

    const html = renderContentBlock(block);

    assert(html.includes('thinking-label'), 'should have thinking label');
    assert(html.includes('let me think...'), 'should contain thinking text');
});

test('renderContentBlock renders tool_use block', () => {
    const block = { type: 'tool_use', text: '', toolName: 'Bash', toolInput: '{"command":"ls"}', toolUseId: 'toolu_123', isError: false };

    const html = renderContentBlock(block);

    assert(html.includes('Bash'), 'should contain tool name');
    assert(html.includes('ls'), 'should contain command');
});

test('renderContentBlock renders tool_result with error', () => {
    const block = { type: 'tool_result', text: 'command not found', toolName: '', toolInput: '', toolUseId: '', isError: true };

    const html = renderContentBlock(block);

    assert(html.includes('Error'), 'should show Error label');
    assert(html.includes('command not found'), 'should contain error text');
    assert(html.includes('error'), 'should have error class');
});

test('renderSession renders title and entries', () => {
    const session = {
        title: 'My Session',
        errors: vec([]),
        entries: vec([
            { type: 'user', timestamp: '', content: vec([{ type: 'text', text: 'hi', toolName: '', toolInput: '', toolUseId: '', isError: false }]) },
            { type: 'assistant', timestamp: '', content: vec([{ type: 'text', text: 'hello', toolName: '', toolInput: '', toolUseId: '', isError: false }]) }
        ])
    };

    const html = renderSession(session);

    assert(html.includes('My Session'), 'should contain title');
    assert(html.includes('hi'), 'should contain user text');
    assert(html.includes('hello'), 'should contain assistant text');
});

test('renderSession renders parse errors', () => {
    const session = {
        title: '',
        errors: vec([{ lineNumber: 3, rawLine: 'bad json' }]),
        entries: vec([])
    };

    const html = renderSession(session);

    assert(html.includes('1 parse error'), 'should show error count');
    assert(html.includes('Line 3'), 'should show line number');
    assert(html.includes('bad json'), 'should show raw line');
});

test('renderSession skips ai-title entry since title is shown separately', () => {
    const session = {
        title: 'My Title',
        errors: vec([]),
        entries: vec([
            { type: 'ai-title', timestamp: '', content: vec([]) },
            { type: 'user', timestamp: '', content: vec([{ type: 'text', text: 'hello', toolName: '', toolInput: '', toolUseId: '', isError: false }]) }
        ])
    };

    const html = renderSession(session);

    assert(!html.includes('ai-title'), 'should not render ai-title entry');
    assert(html.includes('hello'), 'should still render user entry');
});

test('entryClass returns metadata for non-message types', () => {
    assert.strictEqual(entryClass('user'), 'user');
    assert.strictEqual(entryClass('assistant'), 'assistant');
    assert.strictEqual(entryClass('queue-operation'), 'metadata');
    assert.strictEqual(entryClass('file-history-snapshot'), 'metadata');
});

test('escapeHtml escapes special characters', () => {
    assert.strictEqual(escapeHtml('<script>alert("xss")</script>'), '&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;');
});
