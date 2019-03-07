package config;

import org.springframework.context.annotation.*;
import org.springframework.context.support.PropertySourcesPlaceholderConfigurer;
import stresstest.StressTest;

@Configuration
@ComponentScan(basePackageClasses = {StressTest.class})
@PropertySources({
        @PropertySource(value = "classpath:/config/config.properties"),
        @PropertySource(value = "${external.config}", ignoreResourceNotFound = true),
})
public class Config {
    @Bean
    public PropertySourcesPlaceholderConfigurer placeholderConfigurer() {
        return new PropertySourcesPlaceholderConfigurer();
    }
}
