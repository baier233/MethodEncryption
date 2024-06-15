package un.defined.config;

import com.google.gson.Gson;
import lombok.Getter;
import lombok.Setter;

import java.util.Map;

public class ClassMap {

	@Setter
	@Getter
	private static Map<String, Map<String, Double>> classMap;
	public static int getCodeSize(String className, String methodNameAndSign) {
		Map<String, Double> methods = classMap.get(className);
		if (methods != null) {
			Double size = methods.get(methodNameAndSign);
			if (size != null) {
				return size.intValue();
			}
		}
		return -1;
	}
}
