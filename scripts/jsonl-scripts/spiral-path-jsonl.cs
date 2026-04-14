static void CreateSpiralJSONL(SolidWorksApplication swApp, Model model, string outputPath, double scalingFactor = 0.001) {
    using var logger = GeometryLogger.Create(outputPath);
    var bodies = model.AsPart().GetBodies(swBodyType_e.swSolidBody, true);
    var body = bodies.OrderBy(b => b.GetMassProperties(1).Volume).Last();
    var le = GeometryLogEntry.Create("body", "");

    var bBox = body.GetBodyBox(false, false);

    void LOG(IElement e, Color c) {
        le.AddElement(e, "", "", c);
    }

    var lines = bBox.AsLines();

    lines.ForEach((l) => LOG(l, GeometryLogger.Turquoise));

    var longitudinalAxis = lines.OrderBy(l => l.Length).Last();
    var offsetLine = new Line3D(longitudinalAxis.MidPoint(), bBox.Center());
    longitudinalAxis = longitudinalAxis.Move(offsetLine.Vector);

    LOG(longitudinalAxis, GeometryLogger.Turquoise);

    
    const double LAYER_HEIGHT = 0.2 * 4;
    const int NUM_SLICES = 200;


    var range = Enumerable.Range(1, NUM_SLICES).Select(i => i * LAYER_HEIGHT * scalingFactor).ToList();
    var points = range.Select(i => longitudinalAxis.GetPointAtLengthAlongElement(longitudinalAxis.Length - i)).ToList();
    var lgAVector = longitudinalAxis.UnitVector;

    double rA = 0d;
    double rI = 3.5d;

    var yPoint = Point3D.Origin;
    var lastPoint = Point3D.Origin;

    var rayLines = new List<Line3D>();
    foreach (var point in points) {
        LOG(point, GeometryLogger.Green);
        var axis = Vector3D.YAxis;
        var n = (int)(360d / rI);
        var l = new Line3D(lastPoint, point);
        for (var i = 0; i < n; i++) {
            var frac = ((double)i) / (double)n;
            var o = l.GetPointAtFractionAlongElement(frac);
            var dir = Vector3D.XAxis.Rotate(Angle.FromDegrees(rA), axis).Scale(0.01);
            var line = new Line3D(o, o.Move(dir));
            //LOG(line, GeometryLogger.Red);
            rayLines.Add(line);
            rA+=rI;
        }
        lastPoint = point;
    }

    var largestFace = body.GetFaces().OrderBy(f => f.GetArea()).Last();
    var surface = largestFace.GetSurface();

    var mathUtil = swApp.GetMathUtility();
    Point3D lP = default;

    var flip = false;

    foreach (var rayLine in rayLines) {
        var mathPoint = largestFace.GetProjectedPointOn(mathUtil.CreateMathPoint(rayLine.StartPoint, false), mathUtil.CreateMathVector(rayLine.UnitVector, false));
        var p = mathPoint.GetPoint(false);
        if (lP == default) { lP = p; continue; }

        var l = new Line3D(lP, p);

        var hL = new Line3D(l.MidPoint(), l.EndPoint);
        var rotDir = flip ? RotationDirectionE.CounterClockwise : RotationDirectionE.Clockwise;
        hL = hL.Rotate(Angle.QuarterRotation, Vector3D.YAxis, hL.StartPoint, rotDir);

        var arc = Arc3D.FromPoints(l.StartPoint, l.EndPoint, hL.EndPoint);
        LOG(arc, GeometryLogger.Green);

        lP = p;
        flip = !flip;
        Console.WriteLine($"{rayLines.IndexOf(rayLine)}/{rayLines.Count}");
    }

    logger.AddEntry(le);
}