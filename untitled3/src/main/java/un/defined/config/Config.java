package un.defined.config;

import com.google.gson.Gson;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

public class Config {



    private final File configFile;


    public Config(String path){
        this(new File(path));

    }
    public Config(File path){
        this.configFile = path;
    }

    public Settings parseConfig() throws IOException {
        return new Gson().fromJson(Files.readString(configFile.toPath()), Settings.class);
    }

    public static String generateConfig(){
        Settings settings = new Settings("",true,false,false,"");
        return new Gson().toJson(settings,Settings.class);
    }
}
