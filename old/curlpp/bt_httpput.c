#include <stdio.h>
#include <curl/curl.h>

int main(void)
{
	CURL *curl;
	CURLcode res;
	
	curl = curl_easy_init();
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, "http://141.211.203.173:3000/rooms/update_visitors?name=phd_office&mac_addr=39:39:40:10:1E:FF");
		
		res = curl_easy_perform(curl);
		
		// Check for errors
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		
		/* Always clean up */
		curl_easy_cleanup(curl);
		
	}

}