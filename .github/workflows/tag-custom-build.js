const { readFileSync, writeFileSync } = require("fs");
const addJobOutput = require('./add-job-output');
const { GITHUB_SHA } = process.env;

if (GITHUB_SHA) {
  const sha = GITHUB_SHA.slice(0, 7);

  // update cleo.h to replace version string
  const cleoH = readFileSync("cleo_sdk/cleo.h", { encoding: "utf-8" });
  const newCleoH = cleoH
    .replace('(CLEO_VERSION_MAIN.CLEO_VERSION_MAJOR.CLEO_VERSION_MINOR)', `(CLEO_VERSION_MAIN.CLEO_VERSION_MAJOR.CLEO_VERSION_MINOR)"-${sha}"`);
  console.log(`Tagging current build with sha ${sha}`);
  writeFileSync("cleo_sdk/cleo.h", newCleoH, { encoding: "utf-8" });

  const [_, CLEO_VERSION_MAIN] = cleoH.match('#define\\s+CLEO_VERSION_MAIN\\s+(\\d+)');
  const [_, CLEO_VERSION_MAJOR] = cleoH.match('#define\\s+CLEO_VERSION_MAJOR\\s+(\\d+)');
  const [_, CLEO_VERSION_MINOR] = cleoH.match('#define\\s+CLEO_VERSION_MINOR\\s+(\\d+)');

  addJobOutput("sha", sha);
  addJobOutput("version", [CLEO_VERSION_MAIN, CLEO_VERSION_MAJOR, CLEO_VERSION_MINOR].join('.'));
}