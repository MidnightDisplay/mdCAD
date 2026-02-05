#!/usr/bin/env node
import puppeteer from 'puppeteer';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(__dirname, '..');
const outputDir = path.resolve(__dirname, 'test-output');
fs.mkdirSync(outputDir, { recursive: true });
const htmlFile = path.resolve(projectRoot, 'build-web/bin/mdCAD.html');

const args = process.argv.slice(2).filter(a => !a.startsWith('--'));
const flags = process.argv.slice(2).filter(a => a.startsWith('--'));
const HEADLESS = !flags.includes('--visible');

console.log('='.repeat(60));
console.log('ImGui Interaction Test');
console.log('='.repeat(60));

const browser = await puppeteer.launch({
    headless: HEADLESS,
    args: ['--no-sandbox', '--disable-setuid-sandbox']
});

const page = await browser.newPage();
await page.setViewport({ width: 800, height: 600 });

// Capture console
page.on('console', msg => console.log(`[CONSOLE] ${msg.text()}`));
page.on('pageerror', err => console.log(`[ERROR] ${err.message}`));

// Helper to get pixel color from a screenshot at a point
async function getPixelColor(x, y) {
    // Take a screenshot and read pixel from the PNG buffer
    const screenshot = await page.screenshot({ encoding: 'binary' });
    // PNG format: 8-byte signature, then chunks. We'll use a simple approach:
    // Parse PNG to get pixel data using pngjs-like parsing
    // For simplicity, decode using canvas in Node context via page
    const color = await page.evaluate(async ({ x, y, base64 }) => {
        return new Promise((resolve) => {
            const img = new Image();
            img.onload = () => {
                const canvas = document.createElement('canvas');
                canvas.width = img.width;
                canvas.height = img.height;
                const ctx = canvas.getContext('2d');
                ctx.drawImage(img, 0, 0);
                const pixel = ctx.getImageData(x, y, 1, 1).data;
                resolve({ r: pixel[0], g: pixel[1], b: pixel[2] });
            };
            img.src = 'data:image/png;base64,' + base64;
        });
    }, { x, y, base64: screenshot.toString('base64') });
    return color;
}

// Helper to drag from one point to another
async function drag(fromX, fromY, toX, toY, steps = 10) {
    await page.mouse.move(fromX, fromY);
    await page.mouse.down();

    for (let i = 1; i <= steps; i++) {
        const x = fromX + (toX - fromX) * (i / steps);
        const y = fromY + (toY - fromY) * (i / steps);
        await page.mouse.move(x, y);
        await new Promise(r => setTimeout(r, 30));
    }

    await page.mouse.up();
}

try {
    console.log('\n--- Loading page ---');
    await page.goto(`file://${htmlFile}`, { waitUntil: 'networkidle0' });
    await new Promise(r => setTimeout(r, 500));

    // Take initial screenshot
    await page.screenshot({ path: path.join(outputDir, 'test-1-initial.png') });
    console.log('Screenshot: test-1-initial.png');

    // Get initial background color (sample from middle of screen)
    let bgColor = await getPixelColor(400, 400);
    console.log(`Initial background: RGB(${bgColor.r}, ${bgColor.g}, ${bgColor.b})`);

    // The ImGui color sliders are approximately at these positions:
    // Based on the screenshot, the sliders are in the top-left window
    // R slider: around x=55, y=47
    // G slider: around x=130, y=47
    // B slider: around x=205, y=47

    // Drag the R (red) slider to the right to increase red
    console.log('\n--- Dragging R slider (increase red) ---');
    await drag(55, 47, 90, 47);
    await new Promise(r => setTimeout(r, 300));

    await page.screenshot({ path: path.join(outputDir, 'test-2-more-red.png') });
    console.log('Screenshot: test-2-more-red.png');

    bgColor = await getPixelColor(400, 400);
    console.log(`After R drag: RGB(${bgColor.r}, ${bgColor.g}, ${bgColor.b})`);

    // Drag the G (green) slider to change green
    console.log('\n--- Dragging G slider (decrease green) ---');
    await drag(130, 47, 110, 47);
    await new Promise(r => setTimeout(r, 300));

    await page.screenshot({ path: path.join(outputDir, 'test-3-less-green.png') });
    console.log('Screenshot: test-3-less-green.png');

    bgColor = await getPixelColor(400, 400);
    console.log(`After G drag: RGB(${bgColor.r}, ${bgColor.g}, ${bgColor.b})`);

    // Drag the B (blue) slider down to decrease blue
    console.log('\n--- Dragging B slider (decrease blue) ---');
    await drag(205, 47, 170, 47);
    await new Promise(r => setTimeout(r, 300));

    await page.screenshot({ path: path.join(outputDir, 'test-4-less-blue.png') });
    console.log('Screenshot: test-4-less-blue.png');

    bgColor = await getPixelColor(400, 400);
    console.log(`After B drag: RGB(${bgColor.r}, ${bgColor.g}, ${bgColor.b})`);

    // Click the color preview square to open color picker
    console.log('\n--- Clicking color preview square ---');
    await page.mouse.click(270, 47);
    await new Promise(r => setTimeout(r, 500));

    await page.screenshot({ path: path.join(outputDir, 'test-5-color-picker.png') });
    console.log('Screenshot: test-5-color-picker.png');

    // Final wait if visible
    if (!HEADLESS) {
        console.log('\n--- Waiting 3s for viewing ---');
        await new Promise(r => setTimeout(r, 3000));
    }

} catch (err) {
    console.error(`[FATAL] ${err.message}`);
}

await browser.close();
console.log('\n' + '='.repeat(60));
console.log('Test completed');
console.log('='.repeat(60));
