package aplay.testffmpeg;

import android.content.pm.ActivityInfo;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary( "native-lib" );
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate( savedInstanceState );
        //setContentView( R.layout.activity_main );

        // Remove Title
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);

        // Full Screen
        getWindow().setFlags( WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN );

        // Horizontal
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE );

        setContentView( R.layout.activity_main );

        // Example of a call to a native method
        TextView tv = (TextView) findViewById( R.id.sample_text );
        String str = "/sdcard/1080.mp4";
        //Open( str,this );
        //tv.setText( stringFromJNI() );
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native boolean Open(String url,Object handle);
}

