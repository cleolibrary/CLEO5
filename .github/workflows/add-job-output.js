const { appendFileSync } = require("fs");

const { GITHUB_OUTPUT } = process.env;
const { EOL } = require("os");

function addOutput(key, value) {
    appendFileSync(GITHUB_OUTPUT, `${key}=${value}${EOL}`, { encoding: "utf-8" });
}

module.exports = addOutput