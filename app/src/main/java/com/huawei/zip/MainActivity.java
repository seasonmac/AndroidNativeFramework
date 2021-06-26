package com.huawei.zip;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
        Button unzipTestButton = (Button) findViewById(R.id.button);
        String zipPath = getPackageResourcePath();
        String targetDir = getCacheDir().getAbsolutePath();
        unzipTestButton.setOnClickListener((view) -> {
            long native_start = System.currentTimeMillis();
            unzip(zipPath, "assets/manager.apk", targetDir);
            Log.i("Native_unzip", "cost time :" + (System.currentTimeMillis() - native_start) + "ms");
            long start = System.currentTimeMillis();
            copyApk(getBaseContext(), "manager.apk", "manager.apk");
            Log.i("Java_unzip", "cost time :" + (System.currentTimeMillis() - start) + "ms");
        });
    }

    private static boolean copyApk(Context ctx, String apkName, String targetApkName) {
        String tmpTargetApkName = targetApkName + ".tmp";
        String path = ctx.getCacheDir() + File.separator;
        File tmpTargetFile = new File(path, tmpTargetApkName);
        try (FileOutputStream outputStream = new FileOutputStream(tmpTargetFile);
             InputStream inputStream = ctx.getAssets().open(apkName);
             BufferedInputStream bufferedInputStream = new BufferedInputStream(inputStream)) {
            int count;
            FileDescriptor fd = null;
            byte[] buffer = new byte[4096];
            while ((count = bufferedInputStream.read(buffer, 0, buffer.length)) != -1) {
                outputStream.write(buffer, 0, count);
                outputStream.flush();
                if (fd == null) {
                    fd = outputStream.getFD();
                }
                fd.sync();
            }

            boolean succeeded = tmpTargetFile.renameTo(new File(path, targetApkName));
            if (!succeeded) {
                Log.w("Java_unzip", "copy " + apkName + " apk from asset failed");
                return false;
            }
            Log.i("Java_unzip", "copy " + apkName + " apk from asset successfully");
            return true;
        } catch (IOException e) {
            Log.e("Java_unzip", "Cannot get apkFile!", e);
        }
        return false;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native boolean unzip(String zipPath, String fileName, String targetDir);
}