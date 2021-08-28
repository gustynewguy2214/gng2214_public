/**
 * The following is the data needed in the packet
 *
 * Ultrasonic info - “Distance: X.XXXXXX meters” (or feet)
 * Depth: “Depth: X.X meters”
 * Temperature (if working): “Temperature: XX.XX C” (or F)
 * Capacity: “Bladder Empty” (or Full)
 * Water detector: “Water detected [no]”
 *
 * “SonDist:_XX.XXXXXX_[m_\ft]
 *  Depth:_X.X_[m_\ft]
 *  Temp:_XX.XX_[C\F]
 *  Cap:_[Empty\Full]
 *  Water:_[No\Yes]”
 */

struct SUB_OPERATING_INFO
{
	double sonicDist;		// decimal value
	double depth;			// decimal value
	double temp;			// decimal value
	char   capacity;		// 'e' for empty or 'f' for full
	char   water;			// 'y' for water detected or 'n' for no
	char   sonDistUnits; 	// 'm' for meters or 'f' for feet
	char   depthUnits;		// 'm' for meters or 'f' for feet
	char   tempUnits;		// 'f' for fahrenheit or 'c' for celcius
	double lights;          // decimal value
	char   showTime;        // '1' for yes
	char   showSensors;     // '1' for yes
	double pressure;
};
