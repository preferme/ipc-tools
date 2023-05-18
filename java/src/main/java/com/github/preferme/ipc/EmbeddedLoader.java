package com.github.preferme.ipc;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

public final class EmbeddedLoader {

    private EmbeddedLoader() {
    }

    public static void closeQuietly(InputStream is) {
        try {
            is.close();
        } catch (IOException e) {
            // don't care
        }
    }

    public static boolean load(String library) {
        try {
            String osName = System.getProperty("os.name");
            String extension = "so";
            String prefix = "lib";
            if (osName.toLowerCase().contains("windows")) {
                osName = "windows";
                extension = "dll";
            }
            if (osName.startsWith("Mac")) {
                osName = "macos";
                extension = "dylib";
            }
            String osArch = System.getProperty("os.arch");
            osArch = osArch.replace('_', '-');
//            String path = String.format("/WEB-INF/lib/%s/%s/%s%s.%s", osName, osArch, prefix, library, extension);
            String path = String.format("/META-INF/lib/%s%s.%s", prefix, library, extension);

//            java.net.URL url = EmbeddedLoader.class.getResource(path);
//            if ("file".equals(url.getProtocol())) {
//                System.load(url.getFile());
//                return  true;
//            }

            InputStream src = EmbeddedLoader.class.getResourceAsStream(path);

            if (src == null) {
                System.err.println(String.format("Error: No native support for library %s on %s %s", library, osName, osArch));
                return false;
            }

            Path dstFile = Files.createTempFile(library + "-", ".tmp");
            Files.copy(src, dstFile, StandardCopyOption.REPLACE_EXISTING);
            closeQuietly(src);

            System.load(dstFile.toString());

            // NOTE: not attempting to delete tmp file since it will fail anyway on
            // windows (due to the file being locked)
            return true;
        } catch (Exception e) {
            System.err.println(String.format("Error loading library %s: %s", library, e.getMessage()));
            e.printStackTrace();
        }
        return false;
    }
}
