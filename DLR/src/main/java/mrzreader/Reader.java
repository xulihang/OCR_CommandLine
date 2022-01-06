package mrzreader;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

import org.zeromq.SocketType;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;

import com.alibaba.fastjson.JSON;
import com.dynamsoft.dlr.DLRResult;
import com.dynamsoft.dlr.LabelRecognizer;
import com.dynamsoft.dlr.LabelRecognizerException;

public class Reader {

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		LabelRecognizer dlr = null;
		try {
		    LabelRecognizer.initLicense("DLS2eyJvcmdhbml6YXRpb25JRCI6IjEwMDIyNzc2MyJ9");
		    dlr = new LabelRecognizer();
			dlr.appendSettingsFromFile("./wholeImgMRZTemplate.json");
		} catch (LabelRecognizerException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		if (args.length==1) {
			//String filepath = "C:\\Users\\admin\\Pictures\\black_qr_code.png";
			String filepath = args[0];
			File f = new File(filepath);
	        if (f.exists()) {
				String json = recognize(dlr, filepath);
				System.out.println(json);
	        }
			return;
		}else {
			System.out.println("Running under server mode.");
			try (ZContext context = new ZContext()) {
		    	// Socket to talk to clients
		    	ZMQ.Socket socket = context.createSocket(SocketType.REP);
		    	socket.bind("tcp://*:5558");

	            while (true) {
	                // Block until a message is received
	                byte[] reply = socket.recv(0);
                    String s = new String(reply, ZMQ.CHARSET);
	                // Print the message
	                System.out.println(
	                    "Received: [" + s + "]"
	                );
	                File f = new File(s);
	                String response = "Received";
                    if (f.exists()) {
                    	String json = recognize(dlr, s);
                    	response = json;
                    }
	                socket.send(response.getBytes(ZMQ.CHARSET), 0);
	                
	                if (s == "q") {
	                	break;
	                }
	            }
	        }
		}
	}
	
	private static String recognize(LabelRecognizer dlr, String imagePath) {
		ArrayList<HashMap<String,Object>> boxes = new ArrayList<>();
		HashMap<String,Object> result = new HashMap<String, Object>();
		HashMap<String,Object> box = new HashMap<String, Object>();
		long startTime = System.currentTimeMillis();
		try {

			DLRResult[] results = dlr.recognizeByFile(imagePath,"locr");

            StringBuilder sb = new StringBuilder();
            if (results != null && results.length > 0){
                if (results[0].lineResults.length == 2){
                    String MRZCode_1 = results[0].lineResults[0].text;
                    String MRZCode_2 = results[0].lineResults[1].text;
                    sb.append(MRZCode_1);
                    sb.append("\n");
                    sb.append(MRZCode_2);
                    box.put("text", sb.toString());
                    boxes.add(box);
                }else {
                    System.out.println("Line number not matched.");
                }
            }else {
                System.out.println("No result.");
            }
            
		} catch (LabelRecognizerException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		long endTime = System.currentTimeMillis();
        long spentTime = endTime-startTime;
        result.put("boxes", boxes);
        result.put("elapsedTime", spentTime);
        String jsonString = JSON.toJSONString(result);
		return jsonString;
	}

}
