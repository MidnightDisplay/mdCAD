#!/usr/bin/env node
import puppeteer from 'puppeteer';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(__dirname, '..');
const outputDir = path.resolve(__dirname, 'test-output');
fs.mkdirSync(outputDir, { recursive: true });

// Configuration
const args = process.argv.slice(2).filter(a => !a.startsWith('--'));
const flags = process.argv.slice(2).filter(a => a.startsWith('--'));
const HTML_PATH = args[0] || 'build-web/bin/mdCAD.html';
const RUN_TIME = parseInt(args[1]) || 5000; // ms
const SCREENSHOT = flags.includes('--screenshot');
const HEADLESS = !flags.includes('--visible');

const htmlFile = path.resolve(projectRoot, HTML_PATH);
const fileUrl = `file://${htmlFile}`;

console.log('='.repeat(60));
console.log('WASM Debug Session');
console.log('='.repeat(60));
console.log(`File: ${htmlFile}`);
console.log(`Runtime: ${RUN_TIME}ms`);
console.log(`Mode: ${HEADLESS ? 'headless' : 'visible'}`);
console.log('='.repeat(60));

const browser = await puppeteer.launch({
    headless: HEADLESS,
    args: ['--no-sandbox', '--disable-setuid-sandbox']
});

const page = await browser.newPage();

// Capture console messages
page.on('console', msg => {
    const type = msg.type().toUpperCase().padEnd(7);
    console.log(`[${type}] ${msg.text()}`);
});

// Capture errors
page.on('pageerror', err => {
    console.log(`[ERROR  ] ${err.message}`);
});

// Capture request failures
page.on('requestfailed', req => {
    console.log(`[REQFAIL] ${req.url()} - ${req.failure()?.errorText}`);
});

try {
    console.log('\n--- Page Load ---');
    await page.goto(fileUrl, { waitUntil: 'networkidle0', timeout: 30000 });
    console.log('--- Page Loaded ---\n');

    // Let the app run
    await new Promise(r => setTimeout(r, RUN_TIME));

    if (SCREENSHOT) {
        const screenshotPath = path.resolve(outputDir, 'debug-screenshot.png');
        await page.screenshot({ path: screenshotPath });
        console.log(`\nScreenshot saved: ${screenshotPath}`);
    }

} catch (err) {
    console.error(`[FATAL  ] ${err.message}`);
}

await browser.close();
console.log('\n' + '='.repeat(60));
console.log('Debug session ended');
console.log('='.repeat(60));
