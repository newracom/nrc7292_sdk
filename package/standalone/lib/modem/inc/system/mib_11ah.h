#ifndef __MIB_11AH_H__
#define __MIB_11AH_H__

// features
#if !defined(dot11PV1MACHeaderOptionImplemented)
#define dot11PV1MACHeaderOptionImplemented 		0
#endif /* dot11PV1MACHeaderOptionImplemented */

#if !defined(dot11NonTIMModeActivated)
#define dot11NonTIMModeActivated 				1
#endif /* dot11NonTIMModeActivated */

#if !defined(dot11TWTOptionActivated)
#if defined(INCLUDE_TWT_SUPPORT)
#define dot11TWTOptionActivated 				1
#else
#define dot11TWTOptionActivated 				0
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#endif /* dot11TWTOptionActivated */

#if !defined(dot11AMPDUImplemented)
#define dot11AMPDUImplemented 					1
#endif /* dot11AMPDUImplemented */

#if !defined(dot11MaxAMPDULengthExponent)
#define dot11MaxAMPDULengthExponent				1
#endif /* dot11MaxAMPDULengthExponent */

#if !defined(dot11NDPPSPollSupport)
#define dot11NDPPSPollSupport 					0
#endif /* dot11NDPPSPollSupport */

// features, but not in s1g IE
#if !defined(dot11NDPProbingActivated)
#define dot11NDPProbingActivated 				0	// (0:Normal, 1:NDP) Default Disable
#endif /* dot11NDPProbingActivated */

#if !defined(nrcS1GNdpProbeResType)
#define nrcS1GNdpProbeResType					0	// ( 0: PV0, 1: PV1 )
#endif /* nrcS1GNdpProbeResType */

// values
#if !defined(dot11S1GLongOptionActivated)
#define dot11S1GLongOptionActivated 			1
#endif /* dot11S1GLongOptionActivated */

#if !defined(dot11S1GChannelWidthOptionImplemented)
#define dot11S1GChannelWidthOptionImplemented 	1
#endif /* dot11S1GChannelWidthOptionImplemented */

#if !defined(dot11ShortGIOptionIn1MActivated)
#define dot11ShortGIOptionIn1MActivated 		1
#endif /* dot11ShortGIOptionIn1MActivated */

#if !defined(dot11ShortGIOptionIn2MActivated)
#define dot11ShortGIOptionIn2MActivated 		1
#endif /* dot11ShortGIOptionIn2MActivated */

#if !defined(dot11ShortGIOptionIn4MActivated)
#define dot11ShortGIOptionIn4MActivated 		1
#endif /* dot11ShortGIOptionIn4MActivated */

// values, but not defined in MIB
#if !defined(nrcAPStaTypeSupport)
#define nrcAPStaTypeSupport 					0	// ( 0: mixed, 1: sensor, 2: non-sensor )
#endif /* nrcAPStaTypeSupport */

#if !defined(nrcSTAStaTypeSupport)
#define nrcSTAStaTypeSupport 					2   // ( 1: sensor STA, 2: non-sensor )
#endif /* nrcSTAStaTypeSupport */

#if !defined(nrcDuplicate1MHzSupport)
#define nrcDuplicate1MHzSupport 				0
#endif /* nrcDuplicate1MHzSupport */

#if !defined(nrc1MHzControlResponsePreambleSupport)
#define nrc1MHzControlResponsePreambleSupport 	0	//(0: not support, 1:support)
#endif /* nrc1MHzControlResponsePreambleSupport */

// not in both of s1g IE and MIB
#if !defined(nrcTIMEncodingMode)
#define nrcTIMEncodingMode						0	//(0: Block Bitmap, 1: Single AID, 2: OLB, 
#endif /* nrcTIMEncodingMode */
                                              		// 4: Inverse + Block Bitmap, 5: Inverse + Single AID 6: Inverse + OLB) 

#if !defined(nrcS1GMaxMCS1NSS)
#define nrcS1GMaxMCS1NSS						1 	// (0: QPSK, 1: 64QAM, 2: 256QAM, 3: Not Support)
#endif /* nrcS1GMaxMCS1NSS */

#if !defined(nrcS1GMaxMCS2NSS)
#define nrcS1GMaxMCS2NSS						3
#endif /* nrcS1GMaxMCS2NSS */

#if !defined(nrcS1GMaxMCS3NSS)
#define nrcS1GMaxMCS3NSS						3
#endif /* nrcS1GMaxMCS3NSS */

#if !defined(nrcS1GMaxMCS4NSS)
#define nrcS1GMaxMCS4NSS						3
#endif /* nrcS1GMaxMCS4NSS */

#if !defined(nrcS1GMinMCS1NSS)
#define nrcS1GMinMCS1NSS						0 	// (0: No restrict, 1: MCS0 not recommend, 2: MCS0/1 not recommend)
#endif /* nrcS1GMinMCS1NSS */

#if !defined(nrcS1GMinMCS2NSS)
#define nrcS1GMinMCS2NSS						0 	
#endif /* nrcS1GMinMCS2NSS */

#if !defined(nrcS1GMinMCS3NSS)
#define nrcS1GMinMCS3NSS						0 	
#endif /* nrcS1GMinMCS3NSS */

#if !defined(nrcS1GMinMCS4NSS)
#define nrcS1GMinMCS4NSS						0
#endif /* nrcS1GMinMCS4NSS */

#endif //__MIB_11AH_H__
