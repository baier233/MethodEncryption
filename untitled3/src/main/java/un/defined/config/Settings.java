package un.defined.config;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
@Getter
@AllArgsConstructor
public class Settings {
    @Setter
    private String ANNOTATION_DESC;
    @Setter
    private boolean REMOVE_ANNOTATION;
    @Setter
    private String CLASS_HEADER_PATH;
}
