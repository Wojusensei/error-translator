package main

// 调用本机 error-translator 服务的命令行示例
// 注意：输入用 bufio 读取整行，带空格的报错信息不会被截断

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
	"strings"
)

type alternative struct {
	Lang    string `json:"lang"`
	Match   string `json:"match"`
	Level   string `json:"level"`
	Explain string `json:"explain"`
}

type result struct {
	Found    bool   `json:"found"`
	Lang     string `json:"lang"`
	Level    string `json:"level"`
	Category string `json:"category"`
	Explain  string `json:"explain"`
	Source   *struct {
		File string `json:"file"`
		Line int    `json:"line"`
	} `json:"source"`
	Alternatives []alternative `json:"alternatives"`
}

func main() {
	fmt.Print("Enter error message: ")
	text, _ := bufio.NewReader(os.Stdin).ReadString('\n')
	text = strings.TrimSpace(text)

	resp, err := http.Get("http://127.0.0.1:8888/translate?q=" + url.QueryEscape(text))
	if err != nil {
		fmt.Println("[ERROR] Server not running qwq")
		os.Exit(1)
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Println("[ERROR] 读取响应失败:", err)
		os.Exit(1)
	}

	var r result
	if err := json.Unmarshal(body, &r); err != nil {
		fmt.Println("[ERROR] 服务返回的不是合法 JSON:", string(body))
		os.Exit(1)
	}
	if !r.Found {
		fmt.Println("未命中规则")
		return
	}
	fmt.Printf("[%s] %s  类别: %s\n", r.Lang, r.Level, r.Category)
	if r.Source != nil {
		fmt.Printf("出错位置: %s:%d\n", r.Source.File, r.Source.Line)
	}
	fmt.Println(r.Explain)
	for _, alt := range r.Alternatives {
		first := strings.Split(alt.Match, "||")[0]
		fmt.Printf("  其他可能: [%s] %s - %s\n", alt.Lang, first, alt.Explain)
	}
}
