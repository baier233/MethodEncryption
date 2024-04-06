package un.defined.entity;

import lombok.AllArgsConstructor;
import lombok.Getter;

@Getter
@AllArgsConstructor
public class Tuple<TF, TS> {

    private TF first;

    private TS second;

}
