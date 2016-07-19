package com.vrseen;

import android.support.test.runner.AndroidJUnit4;
import android.test.suitebuilder.annotation.SmallTest;
import static org.hamcrest.Matchers.is;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class SourceTest {

	static {
		System.loadLibrary("unittest");
	}

	private native int exec();

	@Test
	public void nativeTest() {
		int exitCode = exec();
		assertThat(exitCode, is(0));
	}
}
