const fs = require("fs");
const title = process.argv[2];

// Step 1: Read, sort, and deduplicate lines
let lines = fs.readFileSync("output.log", "utf8").split(/\r?\n/);
lines.sort();
lines = new Set(lines);

// Step 2 & 3: Replace patterns and process each line for GitHub Actions output (from memory)
let hadOutput = false;
for (let line of lines) {
  line = line.replace(/     \d>/g, "").replace(/.:\\a\\plugin-sdk\\plugin-sdk\\/g, "");
  if (!line.trim()) continue;
  hadOutput = true;
  const match = line.match(/^(.*)\((\d+),(\d+)\): (\w+) (.*) \[.*\]$/);
  if (match) {
    // ::level file=filepath,line=linenumber,title=title::message
    console.log(`::${match[4]} file=${match[1]},line=${match[2]},title=${title}::${match[5]}`);
  } else {
    console.log(`::error title=${title}::${line}`);
  }
}

// Step 4: Exit with error if there was anything in log (error or warning)
if (hadOutput) process.exit(1);