const { appendFileSync, readFileSync, writeFileSync } = require("fs");
const { EOL } = require("os");
const { GITHUB_OUTPUT, GITHUB_REF_NAME } = process.env;

if (GITHUB_REF_NAME) {
  const version = GITHUB_REF_NAME.startsWith("v") ? GITHUB_REF_NAME.slice(1) : GITHUB_REF_NAME;
  addOutput("version", version);
  addOutput("archive_name", `CLEO_${GITHUB_REF_NAME}.zip`);

  // update cleo.h to replace version
  const cleoH = readFileSync("cleo_sdk/cleo.h", { encoding: "utf-8" });

  const [, main, major, minor] = version.match(/(\d+)\.(\d+)\.(\d+).*/);

  const newCleoH = cleoH
    .replace(/#define\s+CLEO_VERSION_MAIN\s+.*/, `#define CLEO_VERSION_MAIN ${main}`)
    .replace(/#define\s+CLEO_VERSION_MAJOR\s+.*/, `#define CLEO_VERSION_MAJOR ${major}`)
    .replace(/#define\s+CLEO_VERSION_MINOR\s+.*/, `#define CLEO_VERSION_MINOR ${minor}`)
    .replace(/#define\s+CLEO_VERSION_STR\s+.*/, `#define CLEO_VERSION_STR "${version}"`);
  writeFileSync("cleo_sdk/cleo.h", newCleoH, { encoding: "utf-8" });
}

const changelog = readFileSync("CHANGELOG.md", { encoding: "utf-8" });
writeFileSync("changes.txt", getChanges().join(EOL), { encoding: "utf-8" });

function addOutput(key, value) {
  appendFileSync(GITHUB_OUTPUT, `${key}=${value}${EOL}`, { encoding: "utf-8" });
}

function getChanges() {
  const lines = changelog.split(EOL);
  const result = [];
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    if (line.trimStart().startsWith("## ")) {
      for (let j = i + 1; j < lines.length; j++) {
        const line = lines[j];
        if (line.trimStart().startsWith("## ")) {
          return result;
        }
        result.push(line);
      }
    }
  }

  return result;
}
