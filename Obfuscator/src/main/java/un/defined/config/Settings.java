package un.defined.config;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
@Getter
@Setter
@AllArgsConstructor
public class Settings {
    private String ANNOTATION_DESC;

    private boolean REMOVE_ANNOTATION;

    private boolean DUMP_TYPE;

    private boolean DONT_LOAD_FOR_DUMP;

    private String CLASS_HEADER_PATH;

}
