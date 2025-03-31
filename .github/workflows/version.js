const { readFileSync, writeFileSync } = require("fs");
const { EOL } = require("os");
const { GITHUB_REF_NAME } = process.env;
const addJobOutput = require('./add-job-output');

if (GITHUB_REF_NAME) {
  const version = GITHUB_REF_NAME.startsWith("v") ? GITHUB_REF_NAME.slice(1) : GITHUB_REF_NAME;
  addJobOutput("version", version);

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


function getChanges() {
  const lines = changelog.split(EOL);
  const result = [
    `## Download Instructions`,
    `An ASI loader is required for CLEO 5 to work. CLEO 5 comes pre-packaged with several popular ASI Loaders ([Silent's ASI Loader](https://cookieplmonster.github.io/mods/gta-sa/#asiloader) and [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)).`,
    `#### If you don't have an ASI loader installed already or unsure which one to download:`,
    `- Download [CLEO ${GITHUB_REF_NAME} with Silent's ASI Loader](https://github.com/cleolibrary/CLEO5/releases/download/${GITHUB_REF_NAME}/SA.CLEO-${GITHUB_REF_NAME}+Silent_ASI_Loader.zip)`,
    `#### If you prefer Ultimate ASI Loader:`,
    `- Download [CLEO ${GITHUB_REF_NAME} with Ultimate ASI Loader](https://github.com/cleolibrary/CLEO5/releases/download/${GITHUB_REF_NAME}/SA.CLEO-${GITHUB_REF_NAME}+Ultimate_ASI_Loader.zip)`,
    `#### If you have an ASI loader installed already:`,
    `- Download [this archive](https://github.com/cleolibrary/CLEO5/releases/download/${GITHUB_REF_NAME}/SA.CLEO-${GITHUB_REF_NAME}.zip) which contains ONLY CLEO 5 library and plugins.`,
    `## Installation`,
    `- Unzip the archive to GTA San Andreas game directory.`,
    `## Changelog`,
  ];

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
