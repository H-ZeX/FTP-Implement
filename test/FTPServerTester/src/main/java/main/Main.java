package main;


import config.Config;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;
import stresstest.StressTest;

public class Main {
    public static void main(String[] args) {
        ApplicationContext applicationContext = new AnnotationConfigApplicationContext(Config.class);
        StressTest test = (StressTest) applicationContext.getBean("StressTest");
        test.stressWithoutDataConnection();
    }
}
