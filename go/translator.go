package main

import (
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
)

func main() {
	fmt.Print("Enter error message: ")
	var input string
	fmt.Scanln(&input)

	resp, err := http.Get("http://localhost:8888/translate?q=" + url.QueryEscape(input))
	if err != nil {
		fmt.Println("[ERROR] Server not running qwq")
		os.Exit(1)
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(resp.Body)
	fmt.Println(string(body))
}