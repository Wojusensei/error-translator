import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.Scanner;

/**
 * 调用本机 error-translator 服务的命令行示例。
 * 标准库没有 JSON 解析器，这里直接输出原始 JSON；要结构化展示可自行引入
 * Jackson/Gson 或参考其他语言的示例。
 */
public class Translator {
    private static final String SERVER = "http://127.0.0.1:8888";

    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        System.out.print("Enter error message: ");
        String input = sc.nextLine();
        sc.close();

        String url = SERVER + "/translate?q=" + URLEncoder.encode(input, StandardCharsets.UTF_8);
        HttpClient client = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(3))
                .build();
        HttpRequest req = HttpRequest.newBuilder().uri(URI.create(url)).build();
        try {
            HttpResponse<String> resp = client.send(req, HttpResponse.BodyHandlers.ofString());
            System.out.println(resp.body());
        } catch (java.io.IOException e) {
            System.out.println("[ERROR] Server not running qwq (" + e.getMessage() + ")");
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}
