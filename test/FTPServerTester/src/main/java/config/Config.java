package config;

import org.springframework.context.annotation.*;
import org.springframework.context.support.PropertySourcesPlaceholderConfigurer;
import org.springframework.core.convert.ConversionService;
import org.springframework.core.convert.support.DefaultConversionService;
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

    @Bean
    public ConversionService conversionService() {
        return new DefaultConversionService();
    }
}
