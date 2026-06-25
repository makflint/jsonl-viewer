// Freshness + smoke test for the COMMITTED WebAssembly artifacts.
//
// Loads web/parser.js (the real Emscripten module) and exercises the C++ parser
// across the WASM→JS boundary on the bundled example inputs. The .wasm binary is
// not byte-reproducible across builds, so we verify behaviour instead of bytes:
// if web/parser.wasm is stale relative to a change in src/*.hpp, these behavioural
// assertions diverge from the committed artifact and the test fails — catching a
// silently out-of-date WASM in CI.
const assert = require('assert');
const fs = require('fs');
const path = require('path');

const ParserModule = require(path.join(__dirname, '..', 'web', 'parser.js'));
const example = name => fs.readFileSync(path.join(__dirname, '..', 'examples', name), 'utf8');

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

function entryTypes(session) {
    const types = [];
    for (let i = 0; i < session.entries.size(); i++) types.push(session.entries.get(i).type);
    return types;
}

function columnNames(schema) {
    const names = [];
    for (let i = 0; i < schema.columns.size(); i++) names.push(schema.columns.get(i).name);
    return names;
}

ParserModule().then(mod => {
    console.log('wasm tests:');

    test('committed module loads and exposes the parser API', () => {
        assert.strictEqual(typeof mod.parseSession, 'function', 'parseSession should be a function');
        assert.strictEqual(typeof mod.hasRawSchema, 'function', 'hasRawSchema should be a function');
        assert.strictEqual(typeof mod.getRawSchema, 'function', 'getRawSchema should be a function');
    });

    test('parses the Claude session example into a timeline', () => {
        const session = mod.parseSession(example('claude-session.jsonl'));
        assert.strictEqual(session.errors.size(), 0, 'should parse without errors');
        assert.strictEqual(session.entries.size(), 7, 'should extract 7 entries');
        assert.strictEqual(session.title, 'Add a string-reversal helper in C++', 'should extract ai-title');
        const types = entryTypes(session);
        assert(types.includes('user'), 'should include a user entry');
        assert(types.includes('assistant'), 'should include an assistant entry');
        assert.strictEqual(mod.hasRawSchema(session), false, 'session input has no raw schema');
    });

    test('parses raw JSONL example into a schema', () => {
        const records = mod.parseSession(example('products.jsonl'));
        assert.strictEqual(records.errors.size(), 0, 'should parse without errors');
        assert.strictEqual(records.entries.size(), 24, 'should produce 24 raw entries');
        assert.strictEqual(mod.hasRawSchema(records), true, 'raw records should yield a schema');
        const schema = mod.getRawSchema(records);
        assert.strictEqual(schema.recordCount, 24, 'schema should count 24 records');
        const names = columnNames(schema);
        for (const field of ['sku', 'price_usd', 'tags']) {
            assert(names.includes(field), `schema should include the "${field}" column`);
        }
    });
}).catch(err => {
    console.log('  FAIL: WASM module failed to load');
    console.log(`    ${err && err.message ? err.message : err}`);
    process.exitCode = 1;
});
