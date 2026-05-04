const http = require('http');
const readline = require('readline');

const rl = readline.createInterface({ input: process.stdin, output: process.stdout });

rl.question('Enter error message: ', (input) => {
    http.get(`http://localhost:8888/translate?q=${encodeURIComponent(input)}`, (res) => {
        let data = '';
        res.on('data', chunk => data += chunk);
        res.on('end', () => { console.log(data); rl.close(); });
    }).on('error', () => {
        console.log('[ERROR] Server not running qwq');
        rl.close();
    });
});