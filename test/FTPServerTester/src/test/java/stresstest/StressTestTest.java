package stresstest;

import config.Config;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

import java.io.File;
import java.util.Arrays;
import java.util.concurrent.ExecutionException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration(classes = Config.class)
public class StressTestTest {
    @Autowired
    StressTest stressTest;

    @Test
    public void test1() throws ExecutionException, InterruptedException {
        stressTest.stressWithoutDataConnection();
    }

    @Test
    public void test2() {
        Pattern pattern = Pattern.compile("\\d+,\\d+,\\d+,\\d+,\\d+,\\d+");
        Matcher matcher = pattern.matcher(" 127,0,0,1,111,111 asdfsalf\r\n");
        boolean match = matcher.find();
        assert match;
        System.out.println(matcher.group());
    }

    @Test
    public void testParsePasv() {
        System.out.println(Arrays.toString(stressTest.parsePasv("portasfdsa 127,0,0,1,111,111")));
    }

    @Test
    public void test3() {
        stressTest.handOnCmdConnection();
    }

    @Test
    public void test5() {
        System.out.println("aa\r\n".lastIndexOf("\r\n"));
    }

    @Test
    public void test6() {
        File file = new File("/tmp");
        System.out.println(file.getName());
        System.out.println(file.getParent());
        System.out.println(file.getPath());
    }
}
