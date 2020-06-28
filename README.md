
# About

An audio external for Windows version of Max 8 (Max/MSP) that produces any wavetable specified by a list in the same format as the *function* Max object. Frequency and Phase can be driven by float and signal in the same way the *cycle~* object does, this means you could also set the frequency using a *Phasor~* object as phase. By default, a crossfade of 200 milliseconds is made when another list is sent. You can set the duration of the crossfade up to 10000 ms and choose between *Linear Crossfade*, *Power Crossfade* or *No crossfade*.

<img src="example/fasordemo.gif" width="400">

To work properly, the format listing of the curve parameters needs to start on the first value of the curve you want to set, and needs to have 3 values for each point (y position, x difference, and curve). You can achieve this easily by changing a pair of attributes on the inspector of the *function* object: 
- The **Output Mode** attribute must be set to *List* (instead of *Normal*)
- The **Mode** attribute must be set to *Curve* (instead of Linear)

This project contains, in addition, a prebuilt external fl_fasor~.mxe64 and help file fl_fasor~.maxhelp

------------------------------------------------------

# Versions History

**v0.1**
- **Important**: fixes first point of the curve to be the correct value instead of zero
- **Important**: fadetime attribute now works properly
- Default curve is not weird anymore
- Attributes change in attrui when set by message
- ***known bug***: attributes doesn't change when written in *attrui*, only changes when the number is changed by dragging the mouse ...don't ask me why
