package me.baier;

import un.defined.Breakpoint;

import java.io.*;
import java.net.URL;
import java.util.Objects;

public class NativeHandler {


	static {
		try {
			URL url = NativeHandler.class.getResource("NativeHandler.dll");
			File temp = File.createTempFile("nativehandler", ".dll");
			temp.deleteOnExit();

			try (InputStream in = Objects.requireNonNull(url).openStream();
			     OutputStream out = new FileOutputStream(temp)) {

				byte[] buffer = new byte[1024];
				int length;
				while ((length = in.read(buffer)) != -1) {
					out.write(buffer, 0, length);
				}
			}

			System.load(temp.getAbsolutePath());

		}catch (IOException e) {
			throw new RuntimeException("Failed to load native library", e);
		}
	}

	public static native void register(Class<?> klass);
}
