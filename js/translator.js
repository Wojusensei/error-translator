// 调用本机 error-translator 服务的 Node.js 命令行示例
// 用法：node translator.js  （随后粘贴报错信息回车）
const http = require('http');
const readline = require('readline');

const rl = readline.createInterface({ input: process.stdin, output: process.stdout });

rl.question('Enter error message: ', (input) => {
    http.get(`http://127.0.0.1:8888/translate?q=${encodeURIComponent(input)}`, { timeout: 3000 }, (res) => {
        let data = '';
        res.on('data', chunk => data += chunk);
        res.on('end', () => {
            try {
                const r = JSON.parse(data);
                if (!r.found) { console.log('未命中规则'); rl.close(); return; }
                console.log(`[${r.lang}] ${r.level}  类别: ${r.category || '-'}`);
                if (r.source) console.log(`出错位置: ${r.source.file}:${r.source.line}`);
                console.log(r.explain);
                for (const alt of r.alternatives || []) {
                    console.log(`  其他可能: [${alt.lang}] ${alt.match.split('||')[0]} - ${alt.explain}`);
                }
            } catch (e) {
                console.log('[ERROR] 服务返回的不是合法 JSON:', data.slice(0, 120));
            }
            rl.close();
        });
    }).on('error', () => {
        console.log('[ERROR] Server not running qwq');
        rl.close();
    }).on('timeout', () => {
        console.log('[ERROR] 请求超时（服务没响应）');
        process.destroy ? process.exit(1) : rl.close();
    });
});
