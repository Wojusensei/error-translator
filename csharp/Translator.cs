using System;
using System.Net.Http;
using System.Web;

class Translator {
    static async Task Main(string[] args) {
        Console.Write("Enter error message: ");
        string input = Console.ReadLine() ?? "";

        using HttpClient client = new();
        string url = "http://localhost:8888/translate?q=" + Uri.EscapeDataString(input);
        string result = await client.GetStringAsync(url);

        Console.WriteLine(result);
    }
}