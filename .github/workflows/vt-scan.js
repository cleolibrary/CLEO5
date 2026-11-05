// VirusTotal AV gate for CLEO5.
//
// Uploads (or looks up) the built binaries on VirusTotal, waits for the
// analysis to come back, and fails when a single file is flagged by more than
// VT_MAX_DETECTIONS engines. The baseline is 0: every `malicious` or
// `suspicious` verdict counts as a new detection, there is no allow list.
//
// A file that VirusTotal already knows (same SHA-256) is not re-uploaded, so a
// rebuild that produced identical bytes costs one lookup and returns at once.
//
// usage: node .github/workflows/vt-scan.js <file-or-glob> [...]
//
// env:
//   VT_KEY              VirusTotal API key (required - the gate fails closed)
//   VT_MAX_DETECTIONS   allowed detections per file (default 2, fails above it)
//   VT_TIMEOUT_MS       time budget for the whole scan (default 30 minutes)
//   VT_MIN_INTERVAL_MS  minimum gap between API calls (default 16000 = 3.75
//                       calls/min, just under the free tier's 4/min; set 0 to
//                       go unpaced on a premium key)

const fs = require('fs');
const path = require('path');
const https = require('https');
const crypto = require('crypto');

const API_HOST = 'www.virustotal.com';
const UPLOAD_LIMIT = 32 * 1024 * 1024; // direct upload endpoint cap
const GUI_URL = 'https://www.virustotal.com/gui/file/'; // human report page for a sha256
const MAX_DETECTIONS = Number(process.env.VT_MAX_DETECTIONS || 2);
const TIMEOUT_MS = Number(process.env.VT_TIMEOUT_MS || 30 * 60 * 1000);
const DEADLINE = Date.now() + TIMEOUT_MS;
const KEY = (process.env.VT_KEY || '').trim();

// Main flow first, helpers below. Function declarations are hoisted and
// `main()` is only invoked on the last line, so the top-level constants and the
// rate-limit state below it are already initialised by the time this runs.
async function main() {
  if (!KEY) {
    console.log('::error title=VirusTotal::VT_KEY is not set, the VirusTotal gate cannot run (failing closed). Add the repository secret VT_KEY.');
    process.exit(1);
  }

  const patterns = process.argv.slice(2);
  if (patterns.length === 0) {
    console.log('::error title=VirusTotal::no files given, usage: node vt-scan.js <file-or-glob> [...]');
    process.exit(1);
  }

  const files = patterns.flatMap(expand);
  const missing = patterns.filter((pattern) => expand(pattern).length === 0);

  const rows = [];
  let failed = false;

  for (const pattern of missing) {
    console.log(`::error title=VirusTotal::no files matched ${pattern}`);
    rows.push({ file: pattern, error: 'not found' });
    failed = true;
  }

  for (const file of files) {
    try {
      const row = await scan(file);
      rows.push(row);

      const verdict = row.hits > MAX_DETECTIONS ? 'FAIL' : 'OK';
      console.log(
        `${row.file}: ${row.hits}/${row.total} detections (${row.source}) -> ${verdict}  ${GUI_URL}${row.sha}`,
      );

      if (row.hits > MAX_DETECTIONS) {
        failed = true;
        console.log(
          `::error title=VirusTotal ${row.file}::${row.hits} of ${row.total} engines flag ${row.file}, ` +
            `more than the ${MAX_DETECTIONS} allowed: ${row.engines.join(', ')}`,
        );
      }
    } catch (error) {
      failed = true;
      rows.push({ file: path.basename(file), error: error.message });
      console.log(`::error title=VirusTotal ${path.basename(file)}::${error.message}`);
    }
  }

  writeSummary(rows, failed);
  process.exit(failed ? 1 : 0);
}

// --- helpers ---------------------------------------------------------------

// Rate limiting. The free public API allows 4 requests/minute, 500/day and
// 15.5K/month; a 16 s gap keeps us at 3.75/min, so we pace proactively instead
// of burning the budget (and wall-clock time) on 429 backoff. Premium keys can
// override with VT_MIN_INTERVAL_MS (0 = unpaced).
const pacEnv = process.env.VT_MIN_INTERVAL_MS;
let minIntervalMs = pacEnv === undefined || pacEnv === '' ? 16000 : Number(pacEnv);
let lastCallAt = 0;

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

async function pace() {
  const wait = lastCallAt + minIntervalMs - Date.now();
  if (wait > 0) await sleep(wait);
  lastCallAt = Date.now();
}

function rawRequest(method, urlPath, { body, headers = {} } = {}) {
  return new Promise((resolve, reject) => {
    const req = https.request(
      {
        method,
        host: API_HOST,
        path: '/api/v3' + urlPath,
        headers: { 'x-apikey': KEY, accept: 'application/json', ...headers },
      },
      (res) => {
        const chunks = [];
        res.on('data', (chunk) => chunks.push(chunk));
        res.on('end', () => {
          const buf = Buffer.concat(chunks);
          let json = null;
          try {
            json = JSON.parse(buf.toString('utf8'));
          } catch {
            /* non-JSON body, keep the text for the error message */
          }
          resolve({ status: res.statusCode, headers: res.headers, json, text: buf.toString('utf8') });
        });
      },
    );
    req.on('error', reject);
    req.setTimeout(60000, () => req.destroy(new Error('request timed out')));
    if (body) req.write(body);
    req.end();
  });
}

async function api(method, urlPath, opts = {}) {
  for (let attempt = 1; ; attempt++) {
    if (Date.now() > DEADLINE) throw new Error(`time budget exhausted before ${method} ${urlPath}`);
    await pace();

    let res;
    try {
      res = await rawRequest(method, urlPath, opts);
    } catch (error) {
      if (attempt >= 4) throw error;
      await sleep(5000 * attempt);
      continue;
    }

    if (res.status === 429) {
      // Quota exhaustion (500/day, 15.5K/month on the free tier) answers with
      // the same 429 as a plain rate limit, but no amount of waiting clears it
      // within this run - fail fast instead of burning six 30 s retries.
      const code = res.json?.error?.code || '';
      if (/quota/i.test(code) || /quota/i.test(res.text || '')) {
        throw new Error(`VirusTotal quota exhausted, the scan cannot continue (${describe(res)})`);
      }
    }
    if (res.status === 429 || res.status === 503) {
      if (attempt >= 6) throw new Error(`still rate limited (HTTP ${res.status}) on ${method} ${urlPath}`);
      const retryAfter = Number(res.headers['retry-after']);
      const waitMs = retryAfter > 0 ? retryAfter * 1000 : 30000;
      minIntervalMs = Math.max(minIntervalMs, 16000);
      console.log(
        `::warning title=VirusTotal::HTTP ${res.status} on ${method} ${urlPath}, waiting ${Math.round(waitMs / 1000)}s ` +
          `and pacing calls ${Math.round(minIntervalMs / 1000)}s apart`,
      );
      await sleep(waitMs);
      lastCallAt = Date.now();
      continue;
    }

    return res;
  }
}

function describe(res) {
  const detail = res.json?.error?.message || res.text?.slice(0, 200) || 'no body';
  return `HTTP ${res.status}: ${detail}`;
}

async function lookup(sha) {
  const res = await api('GET', '/files/' + sha);
  if (res.status === 404) return null;
  if (res.status !== 200) throw new Error('lookup failed, ' + describe(res));

  const attributes = res.json?.data?.attributes;
  if (!attributes?.last_analysis_stats) return null;
  return { stats: attributes.last_analysis_stats, results: attributes.last_analysis_results };
}

async function upload(file) {
  const buf = fs.readFileSync(file);
  const boundary = '----cleo5' + crypto.randomBytes(8).toString('hex');
  const head = Buffer.from(
    `--${boundary}\r\nContent-Disposition: form-data; name="file"; filename="${path.basename(file)}"\r\n` +
      'Content-Type: application/octet-stream\r\n\r\n',
    'utf8',
  );
  const tail = Buffer.from(`\r\n--${boundary}--\r\n`, 'utf8');

  const res = await api('POST', '/files', {
    body: Buffer.concat([head, buf, tail]),
    headers: {
      'content-type': `multipart/form-data; boundary=${boundary}`,
      'content-length': String(head.length + buf.length + tail.length),
    },
  });
  if (res.status !== 200 || !res.json?.data?.id) throw new Error('upload failed, ' + describe(res));
  return res.json.data.id;
}

async function waitForAnalysis(analysisId) {
  let delay = 10000;
  for (;;) {
    await sleep(delay);
    const res = await api('GET', '/analyses/' + analysisId);

    if (res.status === 200) {
      const attributes = res.json?.data?.attributes;
      if (attributes?.status === 'completed' && attributes.stats) {
        return { stats: attributes.stats, results: attributes.results };
      }
    } else if (res.status !== 404) {
      throw new Error('analysis poll failed, ' + describe(res));
    }

    delay = Math.min(Math.round(delay * 1.5), 30000);
  }
}

function detections(stats, results) {
  const hits = (stats.malicious || 0) + (stats.suspicious || 0);
  // Match the denominator the VirusTotal UI shows (e.g. "1/71"): engines that
  // cannot parse the file, failed or timed out are not counted either way.
  const total = (stats.malicious || 0) + (stats.suspicious || 0) + (stats.undetected || 0) + (stats.harmless || 0);
  const engines = Object.values(results || {})
    .filter((engine) => engine.category === 'malicious' || engine.category === 'suspicious')
    .map((engine) => engine.engine_name || '?')
    .sort((a, b) => a.localeCompare(b));
  return { hits, total, engines };
}

// Minimal glob support: only `*` inside the last path segment, which is all the
// workflows need (`.../cleo_plugins/*.cleo`).
function expand(pattern) {
  if (!pattern.includes('*')) return fs.existsSync(pattern) ? [pattern] : [];

  const dir = path.dirname(pattern);
  if (!fs.existsSync(dir)) return [];

  const mask = path
    .basename(pattern)
    .split('*')
    .map((part) => part.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'))
    .join('.*');

  return fs
    .readdirSync(dir)
    .filter((name) => new RegExp('^' + mask + '$').test(name))
    .map((name) => path.join(dir, name))
    .sort();
}

async function scan(file) {
  const buf = fs.readFileSync(file);
  const name = path.basename(file);
  if (buf.length > UPLOAD_LIMIT) {
    throw new Error(`${name} is ${buf.length} bytes, larger than the 32 MiB upload limit`);
  }

  const sha = crypto.createHash('sha256').update(buf).digest('hex');
  console.log(`${name}: sha256 ${sha}`);

  let report = await lookup(sha);
  let source = 'existing report';
  if (!report) {
    console.log(`${name}: not known to VirusTotal, uploading and waiting for the analysis`);
    report = await waitForAnalysis(await upload(file));
    source = 'fresh analysis';
  }

  return { file: name, sha, source, ...detections(report.stats, report.results) };
}

function writeSummary(rows, failed) {
  const lines = [
    '## VirusTotal Scan Results',
    '',
    '| file | detections | verdict | flagged by | sha256 (VirusTotal report) |',
    '| --- | --- | --- | --- | --- |',
  ];

  for (const row of rows) {
    if (row.error) {
      lines.push(`| \`${row.file}\` | - | ERROR | ${row.error} | - |`);
      continue;
    }
    const verdict = row.hits > MAX_DETECTIONS ? '**FAIL**' : 'OK';
    lines.push(
      `| \`${row.file}\` | ${row.hits}/${row.total} | ${verdict} | ${row.engines.join(', ') || '-'} | ` +
        `[\`${row.sha.slice(0, 12)}\`](${GUI_URL}${row.sha}) |`,
    );
  }

  lines.push('', failed ? '**Result: gate failed**' : '**Result: gate passed**');
  const summary = lines.join('\n') + '\n';

  if (process.env.GITHUB_STEP_SUMMARY) fs.appendFileSync(process.env.GITHUB_STEP_SUMMARY, summary);
  console.log('\n' + summary);
}

main();
