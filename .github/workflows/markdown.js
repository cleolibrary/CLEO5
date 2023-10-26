const showdown = require('showdown');
const {readFileSync, writeFileSync} = require('fs');

const md = new showdown.Converter({
  literalMidWordUnderscores: true,
  disableForced4SpacesIndentedSublists: true,
  noHeaderId: true,
  completeHTMLDocument: true,
  simplifiedAutoLink: true,
});

const readme = md.makeHtml(readFileSync('README.md', 'utf8'));
const changelog = md.makeHtml(readFileSync('CHANGELOG.md', 'utf8'));

writeFileSync('README.html', readme, 'utf8');
writeFileSync('CHANGELOG.html', changelog, 'utf8');
