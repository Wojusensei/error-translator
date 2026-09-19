using System;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;

class Translator {
    private const string Server = "http://127.0.0.1:8888";

    static async Task Main(string[] args) {
        Console.Write("Enter error message: ");
        string input = Console.ReadLine() ?? "";

        using HttpClient client = new() { Timeout = TimeSpan.FromSeconds(3) };
        string url = $"{Server}/translate?q={Uri.EscapeDataString(input)}";
        try {
            string body = await client.GetStringAsync(url);
            using JsonDocument doc = JsonDocument.Parse(body);
            JsonElement root = doc.RootElement;

            if (!root.GetProperty("found").GetBoolean()) {
                Console.WriteLine("未命中规则");
                return;
            }
            string lang = root.GetProperty("lang").GetString();
            string level = root.GetProperty("level").GetString();
            string category = root.TryGetProperty("category", out var cat) ? cat.GetString() : "-";
            Console.WriteLine($"[{lang}] {level}  类别: {category}");
            if (root.TryGetProperty("source", out var src)) {
                Console.WriteLine($"出错位置: {src.GetProperty("file").GetString()}:{src.GetProperty("line").GetInt32()}");
            }
            Console.WriteLine(root.GetProperty("explain").GetString());
            if (root.TryGetProperty("alternatives", out var alts)) {
                foreach (JsonElement alt in alts.EnumerateArray()) {
                    string first = alt.GetProperty("match").GetString()!.Split("||")[0];
                    Console.WriteLine($"  其他可能: [{alt.GetProperty("lang").GetString()}] {first} - {alt.GetProperty("explain").GetString()}");
                }
            }
        } catch (JsonException) {
            Console.WriteLine("[ERROR] 服务返回的不是合法 JSON");
        } catch (HttpRequestException e) {
            Console.WriteLine($"[ERROR] Server not running qwq ({e.Message})");
        } catch (TaskCanceledException) {
            Console.WriteLine("[ERROR] 请求超时（服务没响应）");
        }
    }
}
