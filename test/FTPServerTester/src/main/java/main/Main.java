package main;


import config.Config;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;
import stresstest.StressTest;

import java.util.concurrent.ExecutionException;

public class Main {
    public static void main(String[] args) throws ExecutionException, InterruptedException {
        ApplicationContext applicationContext = new AnnotationConfigApplicationContext(Config.class);
        StressTest test = (StressTest) applicationContext.getBean("StressTest");
        test.stressWithoutDataConnection();
    }
}
