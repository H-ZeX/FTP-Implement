package stresstest;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.LongAdder;

@Component("StressTest")
public class StressTest {
    private final int HANG_TIME;
    private final String LOGIN_SUCCESS_MSG = "230 Login successful.\r\n";
    private final String CRLF = "\r\n";
    /**
     * only test the cmd connection
     */
    private int maxCmdConnectCnt;
    private ExecutorService executorService;
    private String serverAddr;
    private int serverPort;
    private String userName;
    private String password;

    private LongAdder successAcc = new LongAdder();

    public StressTest(@Value("${StressTest.MaxCmdConnectionCnt}") int maxCmdConnectCnt,
                      @Value("${StressTest.HangTime}") int hangTime,
                      @Value("${Tester.TesterServerAddress}") String serverAddr,
                      @Value("${Tester.ServerPort}") int serverPort,
                      @Value("${Tester.UserName}") String userName,
                      @Value("${Tester.Password}") String password
    ) {
        this.maxCmdConnectCnt = maxCmdConnectCnt;
        this.serverAddr = serverAddr;
        this.serverPort = serverPort;
        this.userName = userName;
        this.password = password;
        this.HANG_TIME = hangTime;
        this.executorService = new ThreadPoolExecutor(
                maxCmdConnectCnt,
                maxCmdConnectCnt,
                Integer.MAX_VALUE, TimeUnit.DAYS,
                new LinkedBlockingQueue<>(maxCmdConnectCnt),
                r -> {
                    Thread thread = new Thread(r);
                    thread.setName("StressTest#" + thread.getId());
                    return thread;
                });
    }

    public void stressWithoutDataConnection() {
        for (int i = 0; i < maxCmdConnectCnt; i++) {
            executorService.execute(this::handOnCmdConnection);
        }
        executorService.shutdown();
        while (true) {
            try {
                executorService.awaitTermination(Integer.MAX_VALUE, TimeUnit.DAYS);
                break;
            } catch (InterruptedException ignore) {
            }
        }
        assert successAcc.sum() == this.maxCmdConnectCnt;
    }

    private void handOnCmdConnection() {
        try (Socket socket = new Socket(serverAddr, serverPort)) {
            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();

            // read the welcome msg
            readResponse(in);

            out.write(("user " + userName + "\r\n").getBytes());
            out.flush();
            // read the user response msg
            readResponse(in);

            out.write(("pass " + password + "\r\n").getBytes());
            out.flush();
            // read the password response msg
            String response = readResponse(in);
            // System.out.println(response);
            assert LOGIN_SUCCESS_MSG.equals(response);
            successAcc.add(1);
            Thread.sleep(HANG_TIME);
            // System.err.println("Thread Sleep End: " + Thread.currentThread().getName());
        } catch (IOException | InterruptedException e) {
            e.printStackTrace();
            assert false;
        }
    }

    private String readResponse(InputStream in) throws IOException {
        byte[] response = new byte[1024];
        StringBuilder builder = new StringBuilder();
        do {
            int hadRead = in.read(response);
            if (hadRead == -1) {
                return builder.toString();
            }
            builder.append(new String(response, 0, hadRead));
        } while (builder.lastIndexOf(CRLF) == -1);
        return builder.toString();
    }
}
