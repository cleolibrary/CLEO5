const { appendFileSync, readFileSync } = require("fs");
const { EOL } = require("os");
const { GITHUB_OUTPUT, GITHUB_REF_NAME } = process.env;

if (GITHUB_REF_NAME) {
  const version = GITHUB_REF_NAME.startsWith("v") ? GITHUB_REF_NAME.slice(1) : GITHUB_REF_NAME;
  addOutput("version", version);
}

const changelog = readFileSync("CHANGELOG.md", { encoding: "utf-8" });
const changes = getChanges();
addOutput("changes", changes.join("\\n"));

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
