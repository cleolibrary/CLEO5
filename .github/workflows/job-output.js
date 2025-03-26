const { appendFileSync } = require("fs");

const { GITHUB_OUTPUT } = process.env;
const { EOL } = require("os");

export function addOutput(key, value) {
    appendFileSync(GITHUB_OUTPUT, `${key}=${value}${EOL}`, { encoding: "utf-8" });
}