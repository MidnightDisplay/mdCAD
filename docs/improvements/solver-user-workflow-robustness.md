## Some cases where the solver stalls:

1. Take the following geometry:
    1. Line AB
    2. Line CD
    3. Line EF
    4. Arc MNO - M - start point, N - end point, O - center

    We make A coincident to C and add an angle of 90 deg between AB and CD. We make CD along X. We make E coincident to C and make EF along Y. We make EF the ArcAxisLine of MNO. We make O coincident to E, M coincident to D and N coincident to B. We effectively have made a closed loop quarter of a circle arrangement in XZ plane with EF serving as the arc axis. 
    
    Observations:
    1. You can move B or D with the gizmo and this effectively changes the arc radius and causes the other line to follow preseving the closed loop nature of the quarter-circle arrangement.
    2. If you add a length constraint to line AB or CD - the line itself and the arc do react to the change in length, however the other line connected to the arc endpoint (CD or AB respectively) does not seem to update if the jump was big, the solver reaches solve max passes and stops. It needs a wiggle or a change in coordinates.
    3. The nature of how we make the center of the arc coincident to the axis line before forcing the axis reorientation is a bit flaky - we probably want to add an explicit coincident. So in our case adding an EF ArcAxisLine of MNO constraint in our case should add an explicit E coincident to O constraint first, follwed immediately by the ArcAxisLine (and remove the implicit coincidence out of it). I fhave seen the implicit coincidence not being respected after some moves in this arrangement. I think same should go for the line end to arc end tangency - explicit coincidence added and solver adapted to handle this correctly.

2. Solver generally struggles with large jumps:
    1. Consider following geometry:
        1. Line AB and Line CD - A coincident to C
        2. Add arc MNO - make M tangent to B - solver max passes reached, the arc does not become tangent to the line. Have to move the line or the arc points close to each other with the gizmo for the solver to "latch on"
        3. Add another tangency between N and D - have to move the line for the tangency to latch
        4. Add another line EF, make E coincident to O (arc center) - all hell breaks loose, solver can't do the jumps, plus the closed loop nature of ABMNDC breaks with movement, one of the line to arc tangencies is always breaking after max solver passes reached.
        5. Adding the ArcAxisLine between EF and MNO further worsens the solver behaviour

3. Consider the following rectangular arrangement of lines in XZ plane:
    1. lines AB CD EF HG all connected point to point in a loop with coincident constraints.
    2. lines AB and EF are set to Along X, lines CD and HG are set to Along Z
    
    Moving any point on this arrangement works correctly - any motion in XZ plane changes the shape and the area of the rectange. motion along Y moves the rectangle as a whole up or down, preserving shape and area.

    If we replace EF 'along X' with 'parallel to AB' and/or HG 'along Z' with 'parallel to CD' - the solver struggles, although in my mind this case should behave the same as the alongX+alongX and alongZ+alongZ.

    If you go back to the non-parallel arrangement and add another similar rectangle arrangement and connect the bottom with the top with some along Y lines to have effectively a cube lattice - moving points with gizmo becomes more sluggish.

My thoughts: Either we need to play around with tolerances/max solve iterations. or make the solver adaptive, by lowering the tolerance when needed - like when doing gizmo move.
Maybe there is a flaw in how the solver prioritizes anchors - that needs a research.

Another issue is with the script editor - currently re-applying the script seems to reset the entity colors to white and it also breaks the contraints - they look like they apply to the correct entity IDs (ecs assigns new ids to them) but I get the `[ERROR] solve Unsupported coincident participants: only point-point is solved in this phase.` which does not seem to make sense - I have tested this just with two lines in the sketch AB and CD with coincidence between A and C (deleting and readding the constraint solves the issue).


