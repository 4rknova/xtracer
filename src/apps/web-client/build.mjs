#!/usr/bin/env node
// Web client bundle script: concatenate + minify JS and CSS, generate index.bundle.html.
// Usage: node build.mjs [--web-root <path>]
// Output: dist/bundle.min.js, dist/styles.min.css, index.bundle.html
// No npm dependencies — minification is delegated to `npx esbuild`.

import { readFileSync, writeFileSync, mkdirSync } from 'fs';
import { join } from 'path';
import { spawnSync } from 'child_process';

const args = process.argv.slice(2);
const rootIdx = args.indexOf('--web-root');
const webRoot = rootIdx !== -1 ? args[rootIdx + 1] : '.';
const outDir  = join(webRoot, 'dist');

mkdirSync(outDir, { recursive: true });

function minify(code, loader) {
    const r = spawnSync('npx', ['--yes', 'esbuild', '--minify', `--loader=${loader}`],
        { input: code, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
    if (r.status !== 0) { process.stderr.write(r.stderr); process.exit(1); }
    return r.stdout;
}

// ---- CSS ----------------------------------------------------------------
// Parse @import order from styles.css, concatenate, minify.
const stylesEntry = readFileSync(join(webRoot, 'styles.css'), 'utf8');
const cssFiles = [...stylesEntry.matchAll(/@import url\("([^"]+)"\)/g)]
    .map(m => m[1].replace(/^\//, ''));

const combinedCss = cssFiles
    .map(f => readFileSync(join(webRoot, f), 'utf8'))
    .join('\n');

const minCss = minify(combinedCss, 'css');
writeFileSync(join(outDir, 'styles.min.css'), minCss);
console.log(`dist/styles.min.css   ${(minCss.length / 1024).toFixed(1)} kB`);

// ---- JS -----------------------------------------------------------------
// Extract <script src="..."> order from index.html, concatenate, minify.
const html = readFileSync(join(webRoot, 'index.html'), 'utf8');
const jsSrcs = [...html.matchAll(/<script\s+src="\/([^"]+)"/g)].map(m => m[1]);

const combinedJs = jsSrcs.map(f => {
    try   { return readFileSync(join(webRoot, f), 'utf8'); }
    catch { console.warn(`  warning: ${f} not found, skipped`); return ''; }
}).join('\n;\n');

const minJs = minify(combinedJs, 'js');
writeFileSync(join(outDir, 'bundle.min.js'), minJs);
console.log(`dist/bundle.min.js    ${(minJs.length / 1024).toFixed(1)} kB`);

// ---- index.bundle.html --------------------------------------------------
// Replace stylesheet link + remove all individual script tags + inject bundle.
const bundledHtml = html
    .replace(
        '<link rel="stylesheet" href="/styles.css">',
        '<link rel="stylesheet" href="/dist/styles.min.css">'
    )
    .replace(/\n[ \t]*<script\s+src="\/[^"]*"><\/script>/g, '')
    .replace('</body>', '  <script src="/dist/bundle.min.js"></script>\n</body>');

writeFileSync(join(webRoot, 'index.bundle.html'), bundledHtml);
console.log('index.bundle.html');
