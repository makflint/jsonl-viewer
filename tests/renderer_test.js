const assert = require('assert');
global.marked = { parse: text => text.replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>') };
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
    assert(html.includes('06:20:15'), 'should contain formatted timestamp');
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

test('renderEntry hides metadata entries by default', () => {
    const entry = { type: 'queue-operation', timestamp: '2026-03-25T06:20:14.840Z', content: vec([]) };

    const html = renderEntry(entry);

    assert(html.includes('hidden'), 'metadata entry should have hidden style');
});

test('renderEntry formats timestamp as human-readable time', () => {
    const entry = {
        type: 'user',
        timestamp: '2026-03-25T06:20:15.000Z',
        content: vec([{ type: 'text', text: 'hi', toolName: '', toolInput: '', toolUseId: '', isError: false }])
    };

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
    const block = { type: 'tool_use', text: '', toolName: 'Bash', toolInput: '{"command":"ls","description":"List files"}', toolUseId: 'toolu_123', isError: false };
    const toolResults = { 'toolu_123': { type: 'tool_result', text: 'file1.txt', toolName: '', toolInput: '', toolUseId: 'toolu_123', isError: false } };

    const html = renderContentBlock(block, toolResults);

    assert(html.includes('List files'), 'should show description');
    assert(html.includes('tool-use-details'), 'should have collapsible details wrapper');
});

test('renderContentBlock renders Bash command in a copyable code element', () => {
    const block = { type: 'tool_use', text: '', toolName: 'Bash', toolInput: '{"command":"ls -la","description":"List files"}', toolUseId: '', isError: false };

    const html = renderContentBlock(block, {});

    assert(html.includes('<code'), 'should render command in a code element');
    assert(html.includes('ls -la'), 'should contain the command');
});

test('renderSession groups tool_result under matching tool_use', () => {
    const session = {
        title: '',
        errors: vec([]),
        entries: vec([
            { type: 'assistant', timestamp: '', content: vec([
                { type: 'tool_use', text: '', toolName: 'Bash', toolInput: '{"command":"ls"}', toolUseId: 'toolu_123', isError: false }
            ])},
            { type: 'user', timestamp: '', content: vec([
                { type: 'tool_result', text: 'file1.txt\nfile2.txt', toolName: '', toolInput: '', toolUseId: 'toolu_123', isError: false }
            ])}
        ])
    };

    const html = renderSession(session);

    assert(html.includes('Bash'), 'should show tool name');
    assert(html.includes('file1.txt'), 'should show tool result');
    assert(!html.includes('class="entry user"'), 'should not render user entry for tool_result-only messages');
});

test('renderSession keeps user entry when it has non-tool_result content', () => {
    const session = {
        title: '',
        errors: vec([]),
        entries: vec([
            { type: 'user', timestamp: '', content: vec([
                { type: 'text', text: 'hello', toolName: '', toolInput: '', toolUseId: '', isError: false },
                { type: 'tool_result', text: 'output', toolName: '', toolInput: '', toolUseId: 'toolu_456', isError: false }
            ])}
        ])
    };

    const html = renderSession(session);

    assert(html.includes('class="entry user"'), 'should keep user entry');
    assert(html.includes('hello'), 'should show text content');
});

test('escapeHtml escapes special characters', () => {
    assert.strictEqual(escapeHtml('<script>alert("xss")</script>'), '&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;');
});

test('renderContentBlock renders markdown in text blocks', () => {
    const block = { type: 'text', text: 'hello **world**', toolName: '', toolInput: '', toolUseId: '', isError: false };

    const html = renderContentBlock(block);

    assert(html.includes('<strong>world</strong>'), 'should render bold markdown as <strong>');
});
