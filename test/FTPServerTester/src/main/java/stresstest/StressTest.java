package stresstest;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.LongAdder;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@Component("StressTest")
public class StressTest {
    private final int HANG_TIME;
    private final int MAX_CMD_CONNECTION_CNT;
    private final int SERVER_PORT;
    private final int TEST_CNT;
    private final String SERVER_IP;
    private final String USERNAME;
    private final String PASSWORD;
    private final String STOR_CMD_DIR;
    // testDirs will be read in multiple threads,
    // It MUST be final to safely publish to other threads
    private final List<String> testDirs;
    private final CompletionService<Void> executorService;
    private final Pattern pasvPattern;

    private final ThreadLocal<Random> localRandom = new ThreadLocal<>();
    private final AtomicInteger cntForStor = new AtomicInteger(0);
    private final AtomicInteger finishCnt = new AtomicInteger(0);
    private final LongAdder successCnt = new LongAdder();

    public StressTest(@Value("${StressTest.TestCnt}") int testCnt,
                      @Value("${StressTest.MaxCmdConnectionCnt}") int maxCmdConnectionCnt,
                      @Value("${StressTest.MaxThreadCnt}") int maxThreadCnt,
                      @Value("${StressTest.HangTime}") int hangTime,
                      @Value("${Tester.TesterServerAddress}") String serverAddr,
                      @Value("${Tester.ServerPort}") int serverPort,
                      @Value("${Tester.UserName}") String userName,
                      @Value("${Tester.Password}") String password,
                      @Value("${Tester.StorTestDir}") String storCmdDir,
                      @Value("${Tester.ListTestDir}") List<String> testDirs
    ) throws IOException {
        this.TEST_CNT = testCnt;
        this.MAX_CMD_CONNECTION_CNT = maxCmdConnectionCnt;
        this.SERVER_IP = serverAddr;
        this.SERVER_PORT = serverPort;
        this.USERNAME = userName;
        this.PASSWORD = password;
        this.HANG_TIME = hangTime;
        this.STOR_CMD_DIR = storCmdDir;
        this.testDirs = testDirs;

        // // TODO uncomment this when release
        // File storDir = new File(storCmdDir);
        // if (storDir.exists() || !storDir.isAbsolute()) {
        //     throw new IllegalArgumentException("MUST make sure the Tester.StorTestDir DOESN'T exist and make sure it is absolute");
        // }
        // if (!storDir.mkdirs()) {
        //     throw new IOException("create Tester.StorTestDir failed");
        // }
        // for (String testDir : testDirs) {
        //     File dir = new File(testDir);
        //     if (dir.exists() || !dir.isAbsolute()) {
        //         throw new IllegalArgumentException(" MUST make sure Tester.ListTestDir DON'T exist, and make sure they are absolute");
        //     }
        //     if (!dir.mkdirs()) {
        //         throw new IOException("create Tester.StorTestDir failed");
        //     }
        // }

        int threadCnt = maxCmdConnectionCnt > maxThreadCnt ? maxThreadCnt : maxCmdConnectionCnt;
        this.executorService = new ExecutorCompletionService<>(new ThreadPoolExecutor(
                threadCnt, threadCnt,
                Integer.MAX_VALUE, TimeUnit.DAYS,
                new LinkedBlockingQueue<>(maxCmdConnectionCnt),
                r -> {
                    Thread thread = new Thread(r);
                    thread.setName("StressTest#" + thread.getId());
                    return thread;
                }));
        this.pasvPattern = Pattern.compile("\\d+,\\d+,\\d+,\\d+,\\d+,\\d+");
    }

    public void stressWithoutDataConnection() throws InterruptedException, ExecutionException {
        for (int i = 0; i < TEST_CNT; i++) {
            for (int j = 0; j < MAX_CMD_CONNECTION_CNT; j++) {
                executorService.submit(() -> {
                    handOnCmdConnection();
                    return null;
                });
            }
            for (int j = 0; j < MAX_CMD_CONNECTION_CNT; j++) {
                executorService.take().get();
            }
            final int expected = this.MAX_CMD_CONNECTION_CNT;
            assert successCnt.sum() == expected : successCnt.sum() + ", " + expected;
            successCnt.reset();
            System.gc();
            Thread.sleep(120 * 1000);
        }
    }

    public void handOnCmdConnection() {
        if (localRandom.get() == null) {
            localRandom.set(new Random());
        }
        try {
            Thread.sleep(localRandom.get().nextInt(1000));
        } catch (InterruptedException ignored) {
        }
        try (Socket socket = new Socket(SERVER_IP, SERVER_PORT)) {
            // socket.setSoTimeout(1024 * 10);
            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(in));
            login(reader, out, socket);
            if (localRandom.get().nextBoolean()) {
                listUsingPasvCmd(reader, out, socket);
            } else {
                listUsingPortCmd(reader, out, socket);
            }
            if (localRandom.get().nextBoolean()) {
                storUsingPortCmd(reader, out, socket);
            } else {
                storUsingPasvCmd(reader, out, socket);
            }
            successCnt.add(1);
            Thread.sleep(HANG_TIME);
            // System.err.println("Thread Sleep End: " + Thread.currentThread().getName());
        } catch (IOException | InterruptedException e) {
            e.printStackTrace();
        } finally {
            int t = finishCnt.addAndGet(1);
            System.out.println("finish: " + t);
        }
    }

    private ServerSocket portCmd(BufferedReader cmdInput, OutputStream cmdOutput, Socket owner) throws IOException, InterruptedException {
        ServerSocket listen = new ServerSocket();
        listen.setReuseAddress(true);
        listen.bind(null, 1);
        int port = listen.getLocalPort();
        cmdOutput.write(("port 127,0,0,1," + port / 256 + "," + port % 256 + "\r\n").getBytes());
        cmdOutput.flush();
        String r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("200") : r;
        return listen;
    }

    private Socket pasvCmd(BufferedReader cmdInput, OutputStream cmdOutput, Socket owner) throws IOException, InterruptedException {
        cmdOutput.write("pasv\r\n".getBytes());
        cmdOutput.flush();
        String r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("227") : r;
        Object[] addr = parsePasv(r);
        return new Socket((String) addr[0], (int) addr[1]);
    }

    private void login(BufferedReader cmdInput, OutputStream cmdOutput, Socket owner) throws IOException, InterruptedException {
        // read the welcome msg
        String r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("220") : r;
        cmdOutput.write(("user " + USERNAME + "\r\n").getBytes());
        cmdOutput.flush();
        r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("331") : r;
        cmdOutput.write(("pass " + PASSWORD + "\r\n").getBytes());
        cmdOutput.flush();
        r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("230") : r;
    }

    private void listUsingPortCmd(BufferedReader cmdInput, OutputStream cmdOutput, Socket owner) throws IOException, InterruptedException {
        ServerSocket listen = portCmd(cmdInput, cmdOutput, owner);
        String dir = testDirs.get(localRandom.get().nextInt(testDirs.size()));
        cmdOutput.write(("list " + dir + "\r\n").getBytes());
        cmdOutput.flush();
        String r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("150") : r;
        Socket socket = listen.accept();
        listen.close();
        consumeUntilEOF(socket.getInputStream());
        socket.close();
        r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("226") : r;
    }

    private void listUsingPasvCmd(BufferedReader cmdInput, OutputStream cmdOutput, Socket owner) throws IOException, InterruptedException {
        Socket socket = pasvCmd(cmdInput, cmdOutput, owner);
        String dir = testDirs.get(localRandom.get().nextInt(testDirs.size()));
        cmdOutput.write(("list " + dir + "\r\n").getBytes());
        cmdOutput.flush();
        String r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("150") : r;
        consumeUntilEOF(socket.getInputStream());
        socket.close();
        r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("226") : r;
    }

    private void storUsingPortCmd(BufferedReader cmdInput, OutputStream cmdOutput, Socket owner) throws IOException, InterruptedException {
        ServerSocket listen = portCmd(cmdInput, cmdOutput, owner);
        String file = STOR_CMD_DIR + "/tmp_" + cntForStor.addAndGet(1);
        cmdOutput.write(("stor " + file + "\r\n").getBytes());
        cmdOutput.flush();
        String r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("150") : r;
        Socket socket = listen.accept();
        listen.close();
        final String testMsg = "testMsg";
        socket.getOutputStream().write(testMsg.getBytes());
        socket.close();
        r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("226") : r;
        FileInputStream input = new FileInputStream(file);
        String fileText = consumeUntilEOF(input);
        input.close();
        assert testMsg.equals(fileText) : fileText;
    }

    private void storUsingPasvCmd(BufferedReader cmdInput, OutputStream cmdOutput, Socket owner) throws IOException, InterruptedException {
        Socket socket = pasvCmd(cmdInput, cmdOutput, owner);
        String file = STOR_CMD_DIR + "/tmp_" + cntForStor.addAndGet(1);
        cmdOutput.write(("stor " + file + "\r\n").getBytes());
        cmdOutput.flush();
        String r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("150") : r;
        final String testMsg = "testMsg";
        socket.getOutputStream().write(testMsg.getBytes());
        socket.close();
        r = readResponse(cmdInput);
        if (r == null) {
            Thread.sleep(1024 * 1024 * 1024);
            while (true) ;
        }
        assert r.startsWith("226") : r;
        FileInputStream input = new FileInputStream(file);
        String fileText = consumeUntilEOF(input);
        input.close();
        assert testMsg.equals(fileText) : fileText;
    }


    public Object[] parsePasv(String str) {
        Matcher matcher = pasvPattern.matcher(str);
        boolean match = matcher.find();
        assert match;
        String addr = matcher.group();
        String[] tmp = addr.split(",");
        assert tmp.length == 6 : Arrays.toString(tmp) + "";
        String ip = String.join(".", tmp[0], tmp[1], tmp[2], tmp[3]);
        int port = Integer.parseInt(tmp[4]) * 256 + Integer.parseInt(tmp[5]);
        return new Object[]{ip, port};
    }

    private String consumeUntilEOF(InputStream in) throws IOException {
        byte[] buf = new byte[1024];
        StringBuilder res = new StringBuilder();
        while (true) {
            int t = in.read(buf);
            if (t <= 0) {
                break;
            }
            res.append(new String(buf, 0, t));
        }
        return res.toString();
    }

    private String readResponse(BufferedReader in) throws IOException {
        return in.readLine();
    }
}
