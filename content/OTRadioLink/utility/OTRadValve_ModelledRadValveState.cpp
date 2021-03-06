#include <stdlib.h>
#include "utility/OTRadValve_AbstractRadValve.h"

#include "OTRadValve_ModelledRadValveState.h"

namespace OTRadValve
{


//  // Proportional implementation, circa 2013--2016.
//
//  // (Well) under temp target: open valve up.
//  if(adjustedTempC < inputState.targetTempC)
//    {
////V0P2BASE_DEBUG_SERIAL_PRINTLN_FLASHSTRING("under temp");
//    // Force to fully open in BAKE mode.
//    // Need debounced bake mode value to avoid spurious slamming open
//    // of the valve as the user cycles through modes.
//    if(inputState.inBakeMode) { return(inputState.maxPCOpen); }
//
//    // Minimum drop in temperature over recent time to trigger 'window open' response; strictly +ve.
//    // Nominally target up 0.25C--1C drop over a few minutes
//    // (limited by the filter length).
//    // TODO-621: in case of very sharp drop in temperature,
//    // assume that a window or door has been opened,
//    // by accident or to ventilate the room,
//    // so suppress heating to reduce waste.
//    //
//    // See one sample 'airing' data set:
//    //     http://www.earth.org.uk/img/20160930-16WWmultisensortempL.README.txt
//    //     http://www.earth.org.uk/img/20160930-16WWmultisensortempL.png
//    //     http://www.earth.org.uk/img/20160930-16WWmultisensortempL.json.xz
//    //
//    // 7h (hall, A9B2F7C089EECD89) saw a sharp fall and recovery, possibly from an external door being opened:
//    // 1C over 10 minutes then recovery by nearly 0.5C over next half hour.
//    // Note that there is a potential 'sensitising' occupancy signal available,
//    // ie sudden occupancy may allow triggering with a lower temperature drop.
//    //[ "2016-09-30T06:45:18Z", "", {"@":"A9B2F7C089EECD89","+":15,"T|C16":319,"H|%":65,"O":1} ]
//    //[ "2016-09-30T06:57:10Z", "", {"@":"A9B2F7C089EECD89","+":2,"L":101,"T|C16":302,"H|%":60} ]
//    //[ "2016-09-30T07:05:10Z", "", {"@":"A9B2F7C089EECD89","+":4,"T|C16":303,"v|%":0} ]
//    //[ "2016-09-30T07:09:08Z", "", {"@":"A9B2F7C089EECD89","+":5,"tT|C":16,"T|C16":305} ]
//    //[ "2016-09-30T07:21:08Z", "", {"@":"A9B2F7C089EECD89","+":8,"O":2,"T|C16":308,"H|%":64} ]
//    //[ "2016-09-30T07:33:12Z", "", {"@":"A9B2F7C089EECD89","+":11,"tS|C":0,"T|C16":310} ]
//    //
//    // 1g (bedroom, FEDA88A08188E083) saw a slower fall, assumed from airing:
//    // initially of .25C in 12m, 0.75C over 1h, bottoming out ~2h later down ~2C.
//    // Note that there is a potential 'sensitising' occupancy signal available,
//    // ie sudden occupancy may allow triggering with a lower temperature drop.
//    //[ "2016-09-30T06:27:30Z", "", {"@":"FEDA88A08188E083","+":8,"tT|C":17,"tS|C":0} ]
//    //[ "2016-09-30T06:31:38Z", "", {"@":"FEDA88A08188E083","+":9,"gE":0,"T|C16":331,"H|%":67} ]
//    //[ "2016-09-30T06:35:30Z", "", {"@":"FEDA88A08188E083","+":10,"T|C16":330,"O":2,"L":2} ]
//    //[ "2016-09-30T06:43:30Z", "", {"@":"FEDA88A08188E083","+":12,"H|%":65,"T|C16":327,"O":2} ]
//    //[ "2016-09-30T06:59:34Z", "", {"@":"FEDA88A08188E083","+":0,"T|C16":325,"H|%":64,"O":1} ]
//    //[ "2016-09-30T07:07:34Z", "", {"@":"FEDA88A08188E083","+":2,"H|%":63,"T|C16":324,"O":1} ]
//    //[ "2016-09-30T07:15:36Z", "", {"@":"FEDA88A08188E083","+":4,"L":95,"tT|C":13,"tS|C":4} ]
//    //[ "2016-09-30T07:19:30Z", "", {"@":"FEDA88A08188E083","+":5,"vC|%":0,"gE":0,"T|C16":321} ]
//    //[ "2016-09-30T07:23:29Z", "", {"@":"FEDA88A08188E083","+":6,"T|C16":320,"H|%":63,"O":1} ]
//    //[ "2016-09-30T07:31:27Z", "", {"@":"FEDA88A08188E083","+":8,"L":102,"T|C16":319,"H|%":63} ]
//    // ...
//    //[ "2016-09-30T08:15:27Z", "", {"@":"FEDA88A08188E083","+":4,"T|C16":309,"H|%":61,"O":1} ]
//    //[ "2016-09-30T08:27:41Z", "", {"@":"FEDA88A08188E083","+":7,"vC|%":0,"T|C16":307} ]
//    //[ "2016-09-30T08:39:33Z", "", {"@":"FEDA88A08188E083","+":10,"T|C16":305,"H|%":61,"O":1} ]
//    //[ "2016-09-30T08:55:29Z", "", {"@":"FEDA88A08188E083","+":14,"T|C16":303,"H|%":61,"O":1} ]
//    //[ "2016-09-30T09:07:37Z", "", {"@":"FEDA88A08188E083","+":1,"gE":0,"T|C16":302,"H|%":61} ]
//    //[ "2016-09-30T09:11:29Z", "", {"@":"FEDA88A08188E083","+":2,"T|C16":301,"O":1,"L":175} ]
//    //[ "2016-09-30T09:19:41Z", "", {"@":"FEDA88A08188E083","+":4,"T|C16":301,"H|%":61,"O":1} ]
//    //
//    // Should probably be significantly larger than MAX_TEMP_JUMP_C16 to avoid triggering alongside any filtering.
//    // Needs to be be a fast enough fall NOT to be triggered by normal temperature gyrations close to a radiator.
//    static constexpr uint8_t MIN_WINDOW_OPEN_TEMP_FALL_C16 = OTV0P2BASE::fnmax(MAX_TEMP_JUMP_C16+2, 5); // Just over 1/4C.
//    // Minutes over which temperature should be falling to trigger 'window open' response; strictly +ve.
//    // TODO-621.
//    // Needs to be be a fast enough fall NOT to be triggered by normal temperature gyrations close to a radiator.
//    // Is capped in practice at the filter length.
//    static constexpr uint8_t MIN_WINDOW_OPEN_TEMP_FALL_M = 13;
//    //
//    // Avoid trying to heat the outside world when a window or door is opened (TODO-621).
//    // This is a short-term tactical response to a persistent cold draught,
//    // eg from a window being opened to ventilate a room manually,
//    // or an exterior door being left open.
//    //
//    // BECAUSE not currently very close to target
//    // (possibly because of sudden temperature drop already from near target)
//    // AND IF system has 'eco' bias (so tries harder to save energy)
//    // and no fast response has been requested
//    // (eg by recent user operation of the controls)
//    // and the temperature is at/above a minimum frost safety threshold
//    // and the temperature is currently falling (over last measurement period)
//    // and the temperature fall over the last few minutes is large
//    // THEN attempt to stop calling for heat immediately and continue to close
//    // (though if inhibited from turning down yet*,
//    // then instead avoid opening any further).
//    // Turning the valve down should also inhibit reopening for a little while,
//    // even once the temperature has stopped falling.
//    //
//    // *This may be because the valve has just recently been closed
//    // in which case some sort of a temperature drop is not astonishing.
//    //
//    // It seems sensible to stop calling for heat immediately
//    // if one of these events seems to be happening,
//    // though that (a) may not stop the boiler and heat delivery
//    // if other rooms are still calling for heat
//    // and (b) may prevent the boiler being started again for a while
//    // even if this was a false alarm,
//    // so may annoy users and make heating control seem erratic,
//    // so only do this in 'eco' mode where permission has been given
//    // to try harder to save energy.
//    //
//    // TODO: could restrict this to when (valvePCOpen >= inputState.minPCOpen)
//    // to save some thrashing and allow lingering.
//    // TODO: could explicitly avoid applying this when valve has recently been
//    // closed to avoid unwanted feedback loop.
//    if(SUPPORTS_MRVE_DRAUGHT &&
//       inputState.hasEcoBias &&
//       (!inputState.fastResponseRequired) && // Avoid subverting recent manual call for heat.
//       (adjustedTempC >= MIN_TARGET_C) &&
//       (getRawDelta() < 0) &&
//       (getRawDelta(MIN_WINDOW_OPEN_TEMP_FALL_M) <= -(int)MIN_WINDOW_OPEN_TEMP_FALL_C16))
//        {
//        setEvent(MRVE_DRAUGHT); // Report draught detected.
//        if(!dontTurndown())
//          {
//          // Try to turn down far enough to stop calling for heat immediately.
//          if(valvePCOpen >= OTRadValve::DEFAULT_VALVE_PC_SAFER_OPEN)
//            { return(OTRadValve::DEFAULT_VALVE_PC_SAFER_OPEN - 1); }
//          // Else continue to close at a reasonable pace.
//          if(valvePCOpen > TRV_SLEW_PC_PER_MIN)
//            { return(valvePCOpen - TRV_SLEW_PC_PER_MIN); }
//          // Else close it.
//          return(0);
//          }
//        // Else at least avoid opening the valve.
//        return(valvePCOpen);
//        }
//
//    // Limit valve open slew to help minimise overshoot and actuator noise.
//    // This should also reduce nugatory setting changes when occupancy (etc)
//    // is fluctuating.
//    // Thus it may take several minutes to turn the radiator fully on,
//    // though probably opening the first third or so will allow near-maximum
//    // heat output in practice.
//    if(valvePCOpen < inputState.maxPCOpen)
//      {
//      // Reduce valve hunting: defer re-opening if recently closed.
//      if(dontTurnup()) { return(valvePCOpen); }
//
//      // True if a long way below target (more than 1C below target).
//      const bool vBelowTarget = (adjustedTempC < inputState.targetTempC-1);
//
//      // Open glacially if explicitly requested
//      // or if temperature overshoot has happened or is a danger,
//      // or if there's likely no one going to care about getting on target
//      // particularly quickly (or would prefer reduced noise).
//      //
//      // If already at least at the expected minimum % open for significant flow,
//      // AND a wide deadband has been allowed by the caller
//      // (eg room dark or filtering is on or doing pre-warm)
//      //   if not way below target to avoid over-eager pre-warm / anticipation
//      //   for example (TODO-467)
//      //     OR
//      //   if filtering is on indicating rapid recent changes or jitter,
//      //   and the last raw change was upwards,
//      // THEN force glacial mode to try to damp oscillations and avoid overshoot
//      // and excessive valve movement (TODO-453).
//      const bool beGlacial = inputState.glacial ||
//          ((valvePCOpen >= inputState.minPCReallyOpen) && inputState.widenDeadband && !inputState.fastResponseRequired &&
//              (
//               // Don't rush to open the valve (GLACIAL_ON_WITH_WIDE_DEADBAND: TODO-467)
//               // if neither in comfort mode nor massively below (possibly already setback) target temp.
//               (inputState.hasEcoBias && !vBelowTarget) ||
//               // Don't rush to open the valve
//               // if temperature is jittery but is moving in the right direction.
//               // FIXME: maybe redundant w/ GLACIAL_ON_WITH_WIDE_DEADBAND and widenDeadband set when isFiltering is true
//               (isFiltering && (getRawDelta() > 0))));
//      if(beGlacial) { return(valvePCOpen + 1); }
//
//      // If well below target (and without a wide deadband),
//      // or needing a fast response to manual input to be responsive (TODO-593),
//      // then jump straight to (just over*) 'moderately open' if less currently,
//      // which should allow flow and turn the boiler on ASAP,
//      // a little like a mini-BAKE.
//      // For this to work, don't set a wide deadband when, eg,
//      // user has just touched the controls.
//      // *Jump to just over moderately-open threshold
//      // to defeat any small rounding errors in the data path, etc,
//      // since boiler is likely to regard this threshold as a trigger
//      // to immediate action.
//      const uint8_t cappedModeratelyOpen = OTV0P2BASE::fnmin(inputState.maxPCOpen, OTV0P2BASE::fnmin((uint8_t)99, (uint8_t)(OTRadValve::DEFAULT_VALVE_PC_MODERATELY_OPEN+TRV_SLEW_PC_PER_MIN_FAST)));
//      if((valvePCOpen < cappedModeratelyOpen) &&
//         (inputState.fastResponseRequired || (vBelowTarget && !inputState.widenDeadband)))
//          {
//          setEvent(MRVE_OPENFAST);
//          return(cappedModeratelyOpen);
//          }
//
//      // Ensure that the valve opens quickly from cold for acceptable response (TODO-593)
//      // both locally in terms of valve position
//      // and also in terms of the boiler responding.
//      // Less fast if already moderately open or with a wide deadband.
//      const uint8_t slewRate =
//          ((valvePCOpen > OTRadValve::DEFAULT_VALVE_PC_MODERATELY_OPEN) || !inputState.widenDeadband) ?
//              TRV_SLEW_PC_PER_MIN : TRV_SLEW_PC_PER_MIN_VFAST;
//      const uint8_t minOpenFromCold = OTV0P2BASE::fnmax(slewRate, inputState.minPCReallyOpen);
//      // Open to 'minimum' likely open state immediately if less open currently.
//      if(valvePCOpen < minOpenFromCold) { return(minOpenFromCold); }
//      // Slew open relatively gently...
//      // Capped at maximum.
//      return(OTV0P2BASE::fnmin(uint8_t(valvePCOpen + slewRate), inputState.maxPCOpen));
//      }
//    // Keep open at maximum allowed.
//    return(inputState.maxPCOpen);
//    }
//
//  // (Well) over temp target: close valve down.
//  if(adjustedTempC > inputState.targetTempC)
//    {
////V0P2BASE_DEBUG_SERIAL_PRINTLN_FLASHSTRING("over temp");
//
//    if(0 != valvePCOpen)
//      {
//      // Reduce valve hunting: defer re-closing if recently opened.
//      if(dontTurndown()) { return(valvePCOpen); }
//
//      // True if just above the the proportional range.
//      // Extend that 'just over' with a widened deadband to reduce movement and noise at night for example (TODO-1027).
//      const bool justOverTemp = (adjustedTempC == inputState.targetTempC+1) ||
//          (inputState.widenDeadband && (adjustedTempC == inputState.targetTempC+2));
//
//      // TODO-453: avoid closing the valve at all
//      // when the temperature error is small and falling,
//      // and there is a widened deadband.
//      if(justOverTemp && inputState.widenDeadband && (getRawDelta() < 0)) { return(valvePCOpen); }
//
//      // TODO-482: glacial close if temperature is jittery and just over target.
//      if(justOverTemp && isFiltering) { return(valvePCOpen - 1); }
//
//      // Continue shutting valve slowly as not yet fully closed.
//      // TODO-117: allow very slow final turn off
//      // to help systems with poor bypass, ~1% per minute.
//      // Special slow-turn-off rules for final part of travel at/below
//      // "min % really open" floor.
//      const uint8_t minReallyOpen = inputState.minPCReallyOpen;
//      const uint8_t lingerThreshold = SUPPORTS_LINGER && (minReallyOpen > 0) ? (minReallyOpen-1) : 0;
//      if(SUPPORTS_LINGER && (valvePCOpen <= lingerThreshold))
//        {
//        // If lingered long enough then do final chunk in one burst to help avoid valve hiss and temperature overshoot.
//        if((DEFAULT_MAX_RUN_ON_TIME_M < minReallyOpen) && (valvePCOpen < minReallyOpen - DEFAULT_MAX_RUN_ON_TIME_M))
//          { return(0); } // Shut valve completely.
//        // Turn down as slowly as reasonably possible to help boiler cool.
//        return(valvePCOpen - 1);
//        }
//
//      // TODO-109: with comfort bias close relatively slowly
//      //     to reduce wasted effort from minor overshoots.
//      // TODO-453: close relatively slowly when temperature error is small
//      //     (<1C) to reduce wasted effort from minor overshoots.
//      // TODO-593: if user is manually adjusting device
//      //     then attempt to respond quickly.
//      if(((!inputState.hasEcoBias) || justOverTemp || isFiltering) &&
//         (!inputState.fastResponseRequired) &&
//         (valvePCOpen > OTV0P2BASE::fnconstrain((uint8_t)(lingerThreshold + TRV_SLEW_PC_PER_MIN_FAST), (uint8_t)TRV_SLEW_PC_PER_MIN_FAST, inputState.maxPCOpen)))
//        { return(valvePCOpen - TRV_SLEW_PC_PER_MIN_FAST); }
//
//      // Else (by default) force to (nearly) off immediately when requested,
//      // ie eagerly stop heating to conserve energy.
//      // In any case percentage open should now be low enough
//      // to stop calling for heat immediately.
//      return(lingerThreshold);
//      }
//
//    // Ensure that the valve is/remains fully shut.
//    return(0);
//    }
//
//  // Within target 1C range.
//
//  // Close to (or at) target: set valve partly open to try to tightly regulate.
//  //
//  // Use currentTempC16 lsbits to set valve percentage for proportional feedback
//  // to provide more efficient and quieter TRV drive
//  // and probably more stable room temperature.
//  // Bigger lsbits value means closer to target from below,
//  // so closer to valve off.
//  // LSbits of temperature above base of proportional adjustment range.
//  const uint8_t lsbits = (uint8_t) (adjustedTempC16 & 0xf);
//  // 'tmp' in range 1 (at warmest end of 'correct' temperature) to 16 (coolest).
//  const uint8_t tmp = 16 - lsbits;
//  static constexpr uint8_t ulpStep = 6;
//  // Get to nominal range 6 to 96, eg valve nearly shut
//  // just below top of 'correct' temperature window.
//  const uint8_t targetPORaw = tmp * ulpStep;
//  // Constrain from below to likely minimum-open value,
//  // in part to deal with TODO-117 'linger open' in lieu of boiler bypass.
//  // Constrain from above by maximum percentage open allowed,
//  // eg for pay-by-volume systems.
//  const uint8_t targetPO = OTV0P2BASE::fnconstrain(targetPORaw,
//      inputState.minPCReallyOpen, inputState.maxPCOpen);
//
//  // Reduce spurious valve/boiler adjustment by avoiding movement at all
//  // unless current temperature error is significant.
//  if(targetPO != valvePCOpen)
//    {
//    // True iff valve needs to be closed somewhat.
//    const bool tooOpen = (targetPO < valvePCOpen);
//    // Compute the minimum/epsilon slew adjustment allowed (the deadband).
//    // Also increase effective deadband if temperature resolution is lower than 1/16th, eg 8ths => 1+2*ulpStep minimum.
//    // Compute real minimum unit in last place.
//    static constexpr uint8_t realMinUlp = 1 + ulpStep;
//    // Minimum slew/error % distance in central range;
//    // should be larger than smallest temperature-sensor-driven step (6)
//    // to be effective; [1,100].
//    // Note: keeping TRV_MIN_SLEW_PC sufficiently high largely avoids
//    // spurious hunting back and forth from single-ulp noise.
//    static constexpr uint8_t TRV_MIN_SLEW_PC = 7;
//    // Compute minimum slew to use with a wider deadband.
//    const uint8_t ls = OTV0P2BASE::fnmax(TRV_SLEW_PC_PER_MIN, (uint8_t) (2 * TRV_MIN_SLEW_PC));
//    const uint8_t ls2 = OTV0P2BASE::fnmin(ls, (uint8_t) (OTRadValve::DEFAULT_VALVE_PC_MODERATELY_OPEN / 2));
//    const uint8_t ls3 = OTV0P2BASE::fnmax(ls2, (uint8_t) (2 + TRV_MIN_SLEW_PC));
//    const uint8_t _minAbsSlew = (uint8_t)(inputState.widenDeadband ? ls3 : TRV_MIN_SLEW_PC);
//    const uint8_t minAbsSlew = OTV0P2BASE::fnmax(realMinUlp, _minAbsSlew);
//    // Note if not calling for heat from the boiler currently.
//    const bool notCallingForHeat =
//        (valvePCOpen < OTRadValve::DEFAULT_VALVE_PC_SAFER_OPEN);
//    if(tooOpen) // Currently open more than required.  Still below target at top of proportional range.
//      {
////V0P2BASE_DEBUG_SERIAL_PRINTLN_FLASHSTRING("slightly too open");
//      // Reduce valve hunting: defer re-closing if recently opened.
//      if(dontTurndown()) { return(valvePCOpen); }
//
//      // True if recent temp movement, possibly in right direction.
//      // Whole system not just static/stuck/jammed.
//      const bool recentMovement =
//          isFiltering || (getRawDelta(filterLength-1) < 0);
//
//      const uint8_t slew = valvePCOpen - targetPO;
//      // Ensure no hunting for ~1ulp temperature wobble.
//      if((notCallingForHeat || recentMovement) && (slew < minAbsSlew))
//          { return(valvePCOpen); }
//
//      // TODO-453: avoid closing the valve at all when the (raw) temperature
//      // is falling, so as to minimise valve movement.
//      // Since the target is the top of the proportional range
//      // then nothing within it requires the temperature to be *forced* down.
//      // Possibly don't apply this rule at the very top of the range in case
//      // filtering is on and the filtered value moves differently to the raw.
//      // Also avoid forcing the boiler to run continuously.
//      const int rise = getRawDelta();
//      if(rise < 0) { return(valvePCOpen); }
//      // Allow indefinite hovering (avoiding valve movement)
//      // when in wide-deadband mode AND
//      // not calling for heat or some sign of recent temperature fall/movement
//      // so as to avoid forcing the boiler to run continuously
//      // and probably inefficiently and noisily.  (TODO-1096)
//      // In case of (eg) a sticky valve and very stable room temperature,
//      // try to force a small undershoot by slowly allowing valve to drift shut,
//      // allowing the valve to shut and the valve/boiler to close/stop.
//      const bool canHover = inputState.widenDeadband &&
//          (notCallingForHeat || recentMovement);
//      // Avoid closing the valve when temperature steady
//      // and not calling for heat from the boiler.  (TODO-453, TODO-1096)
//      if((0 == rise) && canHover) { return(valvePCOpen); }
//
//      // TODO-1026: minimise movement in dark to avoid disturbing sleep
//      // (darkness generally forces wide deadband).  (TODO-1096)
//      if(canHover && (lsbits < 14)) { return(valvePCOpen); }
//
//      // Close glacially if explicitly requested or if temperature undershoot
//      // has happened or is a danger.
//      // Also be glacial if in soft setback which aims to allow temperatures
//      // to drift passively down a little.
//      //   (TODO-451, TODO-467: have darkness only immediately trigger
//      //    a 'soft setback' using wide deadband)
//      // TODO-482: try to deal better with jittery temperature readings.
//      const bool beGlacial = inputState.glacial ||
//          ((inputState.widenDeadband || isFiltering) && (valvePCOpen <= OTRadValve::DEFAULT_VALVE_PC_MODERATELY_OPEN)) ||
//          (lsbits < 8);
//      if(beGlacial) { return(valvePCOpen - 1); }
//
//      if(slew > TRV_SLEW_PC_PER_MIN_FAST)
//          { return(valvePCOpen - TRV_SLEW_PC_PER_MIN_FAST); } // Cap slew rate.
//      // Adjust directly to target.
//      return(targetPO);
//      }
//
//    // if(targetPO > TRVPercentOpen) // Currently open less than required.  Still below target at top of proportional range.
////V0P2BASE_DEBUG_SERIAL_PRINTLN_FLASHSTRING("slightly too closed");
//    // If room is well below target and the valve is in BAKE mode
//    // then immediately open to maximum.
//    // Needs 'debounced' BAKE mode value to avoid spuriously
//    // slamming open the valve while a user cycles through modes.
//    if(inputState.inBakeMode) { return(inputState.maxPCOpen); }
//
//    // Reduce valve hunting: defer re-opening if recently closed.
//    if(dontTurnup()) { return(valvePCOpen); }
//
//    // True if recent temp movement, possibly in right direction.
//    // Whole system not just static/stuck/jammed.
//    const bool recentMovement =
//        isFiltering || (getRawDelta(filterLength-1) > 0);
//
//    const uint8_t slew = targetPO - valvePCOpen;
//    // To to avoid hunting around boundaries of a ~1ulp temperature step.
//    if((notCallingForHeat || recentMovement) && (slew < minAbsSlew))
//        { return(valvePCOpen); }
//
//    // TODO-453: minimise valve movement (and thus noise and battery use).
//    // Keeping the temperature steady anywhere in the target proportional range
//    // while minimising valve movement/noise/etc is a good goal,
//    // so if raw temperatures are rising (or steady) at the moment
//    // then leave the valve as-is.
//    // If fairly near the final target
//    // then also leave the valve as-is (TODO-453 & TODO-451).
//    // TODO-1026: minimise movement in dark to avoid disturbing sleep
//    // (dark indicated with wide deadband).
//    // DHD20161020: reduced lower threshold with wide deadband
//    // from 8 to 2 (cf 12 without).
//    const int rise = getRawDelta();
//    if(rise > 0) { return(valvePCOpen); }
//    // Allow indefinite hovering (avoiding valve movement)
//    // when in wide-deadband mode AND
//    // not calling for heat or some sign of recent temperature rise/movement
//    // so as to avoid forcing the boiler to run continuously
//    // and probably inefficiently and noisily.  (TODO-1096)
//    // In case of (eg) a sticky valve and very stable room temperature,
//    // try to force a small overshoot by slowly allowing valve to drift open,
//    // allowing the valve to shut and the valve/boiler to close/stop.
//    const bool canHover = inputState.widenDeadband &&
//        (notCallingForHeat || recentMovement);
//    if((0 == rise) && canHover) { return(valvePCOpen); }
//    if(canHover && (lsbits > 1)) { return(valvePCOpen); }
//
//    // Open glacially if explicitly requested or if temperature overshoot
//    // has happened or is a danger.
//    // Also be glacial if in soft setback which aims to allow temperatures
//    // to drift passively down a little.
//    //   (TODO-451, TODO-467: have darkness only immediately trigger
//    //    a 'soft setback' using wide deadband)
//    const bool beGlacial = inputState.glacial ||
//        inputState.widenDeadband ||
//        (lsbits >= 8) ||
//        ((lsbits >= 4) && (valvePCOpen > OTRadValve::DEFAULT_VALVE_PC_MODERATELY_OPEN));
//    if(beGlacial) { return(valvePCOpen + 1); }
//
//    // Slew open faster with comfort bias, or when fastResponseRequired (TODO-593, TODO-1069)
//    const uint8_t maxSlew = (!inputState.hasEcoBias || inputState.fastResponseRequired) ? TRV_SLEW_PC_PER_MIN_FAST : TRV_SLEW_PC_PER_MIN;
//    if(slew > maxSlew)
//        { return(valvePCOpen + maxSlew); } // Cap slew rate towards open.
//    // Adjust directly to target.
//    return(targetPO);
//    }



}
