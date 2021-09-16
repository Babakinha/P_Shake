
// https://github.com/NatronGitHub/openfx-misc/blob/master/Position/Position.cpp

/*
TODO:

    Add advanced options:
        octaves,
        persistance,
        lacunarity,
        burstFrequency,
        burstContrast
*/
#pragma region Head

#include <cmath>
#include <cfloat> // DBL_MAX
#include <iostream>

#include "shake2D.hpp"


#include "ofxsCoords.h"
#include "ofxsMacros.h"
#include "ofxsCopier.h"
#include "ofxsPositionInteract.h"

using namespace OFX;

#define kPluginName "Shaky Waky UwU"
#define kPluginGrouping "Transform"
#define kPluginDescription "Shake plugin using perlin noise"
#define kPluginIdentifier "me.Babakinha.P_Shake"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 1
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kSupportsMultipleClipPARs false
#define kSupportsMultipleClipDepths true
#define kRenderThreadSafety eRenderFullySafe

#define kParamAmplitude "amplitude"
#define kParamAmplitudeLabel "Amplitude"
#define kParamAmplitudeHint "How far it shaky"

#define kParamFrequency "frequency"
#define kParamFrequencyLabel "Frequency"
#define kParamFrequencyHint "How fast it shaky"

#define kParamMultiplier "multiplier"
#define kParamMultiplierLabel "Multiplier"
#define kParamMultiplierHint "Multiplier for the position"

// Some hosts (e.g. Resolve) may not support normalized defaults (setDefaultCoordinateSystem(eCoordinatesNormalised))
// #define kParamDefaultsNormalised "defaultsNormalised"

static bool gHostSupportsDefaultCoordinateSystem = true; // for kParamDefaultsNormalised

#pragma endregion Head
#pragma region Plugin
/**
 * Here is the actual plugin (not the factory)
*/
class Shake: public ImageEffect{
    public:
    /** @brief ctor */
    Shake(OfxImageEffectHandle handle): ImageEffect(handle), _dstClip(NULL), _srcClip(NULL), _amplitude(NULL), _frequency(NULL), _multiplier(NULL)
    {
        _dstClip = fetchClip(kOfxImageEffectOutputClipName);
        _srcClip = getContext() == eContextGenerator ? NULL : fetchClip(kOfxImageEffectSimpleSourceClipName);
        _amplitude = fetchDoubleParam(kParamAmplitude);
        _frequency = fetchDoubleParam(kParamFrequency);
        _multiplier = fetchDoubleParam(kParamMultiplier);
        assert(_amplitude);
        assert(_frequency);
        assert(_multiplier);
    }

    private:
        virtual void render(const RenderArguments &args) OVERRIDE FINAL;
        virtual bool isIdentity(const IsIdentityArguments &args, Clip * &identityClip, double &identityTime, int& view, std::string& plane) OVERRIDE FINAL;

        // override the rod call
        virtual bool getRegionOfDefinition(const RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

        // override the roi call
        virtual void getRegionsOfInterest(const RegionsOfInterestArguments &args, RegionOfInterestSetter &rois) OVERRIDE FINAL;

    private:
        // do not need to delete these, the ImageEffect is managing them for us
        Clip *_dstClip;
        Clip *_srcClip;
        DoubleParam* _amplitude;
        DoubleParam* _frequency;
        DoubleParam* _multiplier;

};

/** Overriding the constructor? */
// Shake::Shake(OfxImageEffectHandle handle): ImageEffect(handle) {
//     /*Do something here if you want to*/
// }
// the overridden render function
void Shake::render(const RenderArguments &args)
{
    assert( kSupportsMultipleClipPARs   || !_srcClip || !_srcClip->isConnected() || _srcClip->getPixelAspectRatio() == _dstClip->getPixelAspectRatio() );
    assert( kSupportsMultipleClipDepths || !_srcClip || !_srcClip->isConnected() || _srcClip->getPixelDepth()       == _dstClip->getPixelDepth() );

    // do the rendering
    std::auto_ptr<Image> dst( _dstClip->fetchImage(args.time) );
    if ( !dst.get() ) {
        throwSuiteStatusException(kOfxStatFailed);
    }
    checkBadRenderScaleOrField(dst, args);
    void* dstPixelData;
    OfxRectI dstBounds;
    PixelComponentEnum dstComponents;
    BitDepthEnum dstBitDepth;
    int dstRowBytes;
    getImageData(dst.get(), &dstPixelData, &dstBounds, &dstComponents, &dstBitDepth, &dstRowBytes);
    int dstPixelComponentCount = dst->getPixelComponentCount();
    std::auto_ptr<const Image> src( ( _srcClip && _srcClip->isConnected() ) ?
                                    _srcClip->fetchImage(args.time) : 0 );
    const void* srcPixelData;
    OfxRectI srcBounds;
    PixelComponentEnum srcPixelComponents;
    BitDepthEnum srcBitDepth;
    int srcRowBytes;
    getImageData(src.get(), &srcPixelData, &srcBounds, &srcPixelComponents, &srcBitDepth, &srcRowBytes);
    int srcPixelComponentCount = src.get() ? src->getPixelComponentCount() : 0;

    // translate srcBounds
    const double time = args.time;
    double par = _dstClip->getPixelAspectRatio();
    Putils::Vector2 t_canonical;
    t_canonical = Putils::Shake2D(_amplitude->getValueAtTime(time), _frequency->getValueAtTime(time), 2, 0.2f, 20, 0.5f, 2, time);
    t_canonical.x *= _multiplier->getValueAtTime(time);
    t_canonical.y *= _multiplier->getValueAtTime(time);
    OfxPointI t_pixel;

    // rounding is done by going to pixels, and back to Canonical
    t_pixel.x = (int)std::floor(t_canonical.x * args.renderScale.x / par + 0.5);
    t_pixel.y = (int)std::floor(t_canonical.y * args.renderScale.y + 0.5);
    if (args.fieldToRender == eFieldBoth) {
        // round to an even y
        t_pixel.y = t_pixel.y - (t_pixel.y & 1);
    }

    // translate srcBounds
    srcBounds.x1 += t_pixel.x;
    srcBounds.x2 += t_pixel.x;
    srcBounds.y1 += t_pixel.y;
    srcBounds.y2 += t_pixel.y;

    copyPixels(*this, args.renderWindow, args.renderScale, srcPixelData, srcBounds, srcPixelComponents, srcPixelComponentCount, srcBitDepth, srcRowBytes, dstPixelData, dstBounds, dstComponents, dstPixelComponentCount, dstBitDepth, dstRowBytes);
} // PositionPlugin::render

// override the rod call
bool Shake::getRegionOfDefinition(const RegionOfDefinitionArguments &args, OfxRectD &rod) {
    if (!_srcClip || !_srcClip->isConnected()) {
        return false;
    }
    const double time = args.time;
    OfxRectD srcrod = _srcClip->getRegionOfDefinition(time);
    if ( Coords::rectIsEmpty(srcrod) ) {
        return false;
    }
    double par = _dstClip->getPixelAspectRatio();
    Putils::Vector2 t_canonical;
    t_canonical = Putils::Shake2D(_amplitude->getValueAtTime(time), _frequency->getValueAtTime(time), 2, 0.2f, 20, 0.5f, 2, time);
    t_canonical.x *= _multiplier->getValueAtTime(time);
    t_canonical.y *= _multiplier->getValueAtTime(time);
    OfxPointI t_pixel;

    // rounding is done by going to pixels, and back to Canonical
    t_pixel.x = (int)std::floor(t_canonical.x * args.renderScale.x / par + 0.5);
    t_pixel.y = (int)std::floor(t_canonical.y * args.renderScale.y + 0.5);
    if (_srcClip->getFieldOrder() == eFieldBoth) {
        // round to an even y
        t_pixel.y = t_pixel.y - (t_pixel.y & 1);
    }
    if ( (t_pixel.x == 0) && (t_pixel.y == 0) ) {
        return false;
    }
    t_canonical.x = t_pixel.x * par / args.renderScale.x;
    t_canonical.y = t_pixel.y / args.renderScale.y;
    rod.x1 = srcrod.x1 + t_canonical.x;
    rod.x2 = srcrod.x2 + t_canonical.x;
    rod.y1 = srcrod.y1 + t_canonical.y;
    rod.y2 = srcrod.y2 + t_canonical.y;

    return true;
}

// override the roi call
void Shake::getRegionsOfInterest(const RegionsOfInterestArguments &args, RegionOfInterestSetter &rois) {
    if (!_srcClip || !_srcClip->isConnected()) {
        return;
    }
    const double time = args.time;
    OfxRectD srcRod = _srcClip->getRegionOfDefinition(time);
    if ( Coords::rectIsEmpty(srcRod) ) {
        return;
    }
    double par = _dstClip->getPixelAspectRatio();
    Putils::Vector2 t_canonical;
    t_canonical = Putils::Shake2D(_amplitude->getValueAtTime(time), _frequency->getValueAtTime(time), 2, 0.2f, 20, 0.5f, 2, time);
    t_canonical.x *= _multiplier->getValueAtTime(time);
    t_canonical.y *= _multiplier->getValueAtTime(time);
    OfxPointI t_pixel;

    // rounding is done by going to pixels, and back to Canonical
    t_pixel.x = (int)std::floor(t_canonical.x * args.renderScale.x / par + 0.5);
    t_pixel.y = (int)std::floor(t_canonical.y * args.renderScale.y + 0.5);
    if (_srcClip->getFieldOrder() == eFieldBoth) {
        // round to an even y
        t_pixel.y = t_pixel.y - (t_pixel.y & 1);
    }
    if ( (t_pixel.x == 0) && (t_pixel.y == 0) ) {
        return;
    }
    t_canonical.x = t_pixel.x * par / args.renderScale.x;
    t_canonical.y = t_pixel.y / args.renderScale.y;

    OfxRectD srcRoi = args.regionOfInterest;
    srcRoi.x1 -= t_canonical.x;
    srcRoi.x2 -= t_canonical.x;
    srcRoi.y1 -= t_canonical.y;
    srcRoi.y2 -= t_canonical.y;
    // intersect srcRoi with srcRoD
    Coords::rectIntersection(srcRoi, srcRod, &srcRoi);
    rois.setRegionOfInterest(*_srcClip, srcRoi);
}

// overridden is identity
bool Shake::isIdentity(const IsIdentityArguments &args, Clip * &identityClip, double & /*identityTime*/, int& /*view*/, std::string& /*plane*/) {
    const double time = args.time;
    double par = _dstClip->getPixelAspectRatio();
    Putils::Vector2 t_canonical;

    t_canonical = Putils::Shake2D(_amplitude->getValueAtTime(time), _frequency->getValueAtTime(time), 2, 0.2f, 20, 0.5f, 2, time);
    t_canonical.x *= _multiplier->getValueAtTime(time);
    t_canonical.y *= _multiplier->getValueAtTime(time);
    OfxPointI t_pixel;

    // rounding is done by going to pixels, and back to Canonical
    t_pixel.x = (int)std::floor(t_canonical.x * args.renderScale.x / par + 0.5);
    t_pixel.y = (int)std::floor(t_canonical.y * args.renderScale.y + 0.5);
    if (args.fieldToRender == eFieldBoth) {
        // round to an even y
        t_pixel.y = t_pixel.y - (t_pixel.y & 1);
    }
    if ( (t_pixel.x == 0) && (t_pixel.y == 0) ) {
        identityClip = _srcClip;

        return true;
    }

    return false;
}

#pragma endregion Plugin
#pragma region Factory
mDeclarePluginFactory(ShakePluginFactory, {}, {});

struct ShakeInteractParam
{
    static const char * amplitudeName() { return kParamAmplitude; }

    static const char * frequencyName() { return kParamFrequency; }
};

/**
 * This is where we set plugin propreties
 * E.g: Name, Description, Contexts, pixel depths, some flags etc.
 * 
 * btw, i think Basic Labels and suported context are mandatory
*/
void ShakePluginFactory::describe(ImageEffectDescriptor &desc) {
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextFilter);


    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthNone);
    desc.addSupportedBitDepth(eBitDepthUByte);
    desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthHalf);
    desc.addSupportedBitDepth(eBitDepthFloat);
    desc.addSupportedBitDepth(eBitDepthCustom);

    // set a few flags
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setSupportsTiles(kSupportsTiles);
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);
    desc.setSupportsMultipleClipDepths(kSupportsMultipleClipDepths);
    desc.setRenderThreadSafety(kRenderThreadSafety);

    //desc.setOverlayInteractDescriptor(new PositionOverlayDescriptor<ShakeInteractParam>);
}
/**
 * I think here is where you put settings and things like that
 * E.g: Sliders(doubles), Checkboxes(booleans) etc
 * 
 * And maybe also where you get the clip
*/
void ShakePluginFactory::describeInContext(ImageEffectDescriptor& desc, ContextEnum /*context*/)
{   
    // Source clip only in the filter context
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);

    srcClip->addSupportedComponent(ePixelComponentNone);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    srcClip->addSupportedComponent(ePixelComponentAlpha);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentNone);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->addSupportedComponent(ePixelComponentAlpha);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages and to things in
    PageParamDescriptor *page = desc.definePageParam("Controls");
    bool hostHasNativeOverlayForPosition;
    // Amplitude
    
    DoubleParamDescriptor* ampParam = desc.defineDoubleParam(kParamAmplitude);
    ampParam->setLabel(kParamAmplitudeLabel);
    ampParam->setHint(kParamAmplitudeHint);
    ampParam->setDoubleType(eDoubleTypeAngle);
    if ( ampParam->supportsDefaultCoordinateSystem() ) {
        ampParam->setDefaultCoordinateSystem(eCoordinatesNormalised); // no need of kParamDefaultsNormalised
    } else {
        gHostSupportsDefaultCoordinateSystem = false; // no multithread here, see kParamDefaultsNormalised
    }
    ampParam->setDefault(1.);
    ampParam->setRange(-DBL_MAX, DBL_MAX); // Resolve requires range and display range or values are clamped to (-1,1)
    ampParam->setDisplayRange(-5, 10); // Resolve requires display range or values are clamped to (-1,1)
    hostHasNativeOverlayForPosition = ampParam->getHostHasNativeOverlayHandle();
    if (hostHasNativeOverlayForPosition) {
        ampParam->setUseHostNativeOverlayHandle(true);
    }
    if (page) {
        page->addChild(*ampParam);
    }
    
    // Frequency
    
    DoubleParamDescriptor* freqParam = desc.defineDoubleParam(kParamFrequency);
    freqParam->setLabel(kParamFrequencyLabel);
    freqParam->setHint(kParamFrequencyHint);
    freqParam->setDoubleType(eDoubleTypeAngle);
    if ( freqParam->supportsDefaultCoordinateSystem() ) {
        freqParam->setDefaultCoordinateSystem(eCoordinatesNormalised); // no need of kParamDefaultsNormalised
    } else {
        gHostSupportsDefaultCoordinateSystem = false; // no multithread here, see kParamDefaultsNormalised
    }
    freqParam->setDefault(0.9);
    freqParam->setRange(-DBL_MAX, DBL_MAX); // Resolve requires range and display range or values are clamped to (-1,1)
    freqParam->setDisplayRange(-5, 0.99); // Resolve requires display range or values are clamped to (-1,1)
    hostHasNativeOverlayForPosition = freqParam->getHostHasNativeOverlayHandle();
    if (hostHasNativeOverlayForPosition) {
        freqParam->setUseHostNativeOverlayHandle(true);
    }
    if (page) {
        page->addChild(*freqParam);
    }

    // Multiplier
    
    DoubleParamDescriptor* multParam = desc.defineDoubleParam(kParamMultiplier);
    multParam->setLabel(kParamMultiplierLabel);
    multParam->setHint(kParamMultiplierHint);
    multParam->setDoubleType(eDoubleTypeScale);
    if ( multParam->supportsDefaultCoordinateSystem() ) {
        multParam->setDefaultCoordinateSystem(eCoordinatesNormalised); // no need of kParamDefaultsNormalised
    } else {
        gHostSupportsDefaultCoordinateSystem = false; // no multithread here, see kParamDefaultsNormalised
    }
    multParam->setDefault(250.);
    multParam->setRange(-DBL_MAX, DBL_MAX); // Resolve requires range and display range or values are clamped to (-1,1)
    multParam->setDisplayRange(-1000, 1000); // Resolve requires display range or values are clamped to (-1,1)
    hostHasNativeOverlayForPosition = ampParam->getHostHasNativeOverlayHandle();
    if (hostHasNativeOverlayForPosition) {
        multParam->setUseHostNativeOverlayHandle(true);
    }
    if (page) {
        page->addChild(*multParam);
    }
    
}


/**
 * Creates the Plugin instance duh...
*/
ImageEffect* ShakePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/) {
    return new Shake(handle);
}
static ShakePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p);
#pragma endregion Factory