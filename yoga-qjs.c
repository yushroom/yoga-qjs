#include "yoga-qjs.h"
#include <yoga/Yoga.h>
#include <quickjs.h>
#include <cutils.h>

static int JS_ToFloat32(JSContext *ctx, float *pres, JSValueConst val)
{
	double f;
	int ret = JS_ToFloat64(ctx, &f, val);
	if (ret == 0)
		*pres = (float)f;
	return ret;
}

static JSValue js_wrap_YGValue(JSContext *ctx, YGValue val)
{
	JSValue e = JS_NewObject(ctx);
	JS_DefinePropertyValueStr(ctx, e, "value", JS_NewFloat64(ctx, val.value), JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(ctx, e, "unit", JS_NewInt32(ctx, val.unit), JS_PROP_C_W_E);
	return e;
}

//static JSValue js_wrap_class(JSContext *ctx, void *s, JSClassID classID)
//{
//	if (s == NULL)
//		return JS_NULL;
//	JSValue obj = JS_NewObjectClass(ctx, classID);
//	if (JS_IsException(obj))
//		return obj;
//	JS_SetOpaque(obj, s);
//	return obj;
//}

//typedef struct {
//	YGNodeRef node;
//	BOOL free_in_finalizer;
//} JSYGNode;

static JSClassID js_yoga_node_class_id;

//static JSValue js_wrap_YGNode(JSContext *ctx, YGNodeRef val)
//{
//	return js_wrap_class(ctx, val, js_yoga_node_class_id);
//}

static void js_yoga_node_finalizer(JSRuntime *rt, JSValue val)
{
	YGNodeRef n = JS_GetOpaque(val, js_yoga_node_class_id);
	if (n) {
		YGNodeFree(n);
	}
}

static JSValue js_yoga_node_ctor(JSContext *ctx,
								 JSValueConst new_target,
								 int argc, JSValueConst *argv)
{
	YGNodeRef p;
	JSValue obj = JS_UNDEFINED;
	JSValue proto;
	
	p = YGNodeNew();
	if (!p)
		return JS_EXCEPTION;
	
	proto = JS_GetPropertyStr(ctx, new_target, "prototype");
	if (JS_IsException(proto))
		goto fail;
	obj = JS_NewObjectProtoClass(ctx, proto, js_yoga_node_class_id);
	JS_FreeValue(ctx, proto);
	if (JS_IsException(obj))
		goto fail;
	JS_SetOpaque(obj, p);
	return obj;
fail:
	js_free(ctx, p);
	JS_FreeValue(ctx, obj);
	return JS_EXCEPTION;
}

static JSClassDef js_yoga_node_class = {
	"node",
	.finalizer = js_yoga_node_finalizer,
};

enum YGNODE_PROP{
	E_PositionType = 0,
	E_Position,
	E_PositionPercent,
	
	E_AlignContent,
	E_AlignItems,
	E_AlignSelf,
	E_FlexDirection,
	E_FlexWrap,
	E_JustifyContent,
	
	E_Margin,
	E_MarginPercent,
	E_MarginAuto,
	
	E_Overflow,
	E_Display,
	
	E_Flex,
	E_FlexBasis,
	E_FlexBasisPercent,
	E_FlexGrow,
	E_FlexShrink,
	
	E_Width,
	E_WidthPercent,
	E_WidthAuto,
	E_Height,
	E_HeightPercent,
	E_HeightAuto,
	
	E_MinWidth,
	E_MinWidthPercent,
	E_MinHeight,
	E_MinHeightPercent,
	
	E_MaxWidth,
	E_MaxWidthPercent,
	E_MaxHeight,
	E_MaxHeightPercent,
	
	E_AspectRatio,
	
	E_Border,
	
	E_Padding,
	E_PaddingPercent,
	
	E_Parent,
	E_ChildCount,
	
	E_Dirty,
	
	E_ComputedLeft,
	E_ComputedRight,
	
	E_ComputedTop,
	E_ComputedBottom,
	
	E_ComputedWidth,
	E_ComputedHeight,
	
	E_ComputedLayout,
	
	E_ComputedMargin,
	E_ComputedBorder,
	E_ComputedPadding,
};

static JSValue js_yoga_make_layout(JSContext *ctx, float left, float right, float top, float bottom, float width, float height)
{
	JSValue e = JS_NewObject(ctx);
	JS_DefinePropertyValueStr(ctx, e, "left",   JS_NewFloat64(ctx, left), JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(ctx, e, "right",  JS_NewFloat64(ctx, right), JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(ctx, e, "top",    JS_NewFloat64(ctx, top), JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(ctx, e, "bottom", JS_NewFloat64(ctx, bottom), JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(ctx, e, "width",  JS_NewFloat64(ctx, width), JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(ctx, e, "height", JS_NewFloat64(ctx, height), JS_PROP_C_W_E);
	return e;
}

static JSValue js_yoga_node_getComputedLayout(JSContext *ctx, JSValueConst this_val)
{
	YGNodeRef n = JS_GetOpaque2(ctx, this_val, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	return js_yoga_make_layout(ctx,
							   YGNodeLayoutGetLeft(n),
							   YGNodeLayoutGetRight(n),
							   YGNodeLayoutGetTop(n),
							   YGNodeLayoutGetBottom(n),
							   YGNodeLayoutGetWidth(n),
							   YGNodeLayoutGetHeight(n));
}

static JSValue js_yoga_node_getComputedAbsoluteLayout(JSContext *ctx, JSValueConst this_val)
{
	YGNodeRef n = JS_GetOpaque2(ctx, this_val, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	YGNodeRef p = n;
	float left = 0, right = 0;
	float top = 0, bottom = 0;
	while (p) {
		float l = YGNodeLayoutGetLeft(p);
		float t = YGNodeLayoutGetTop(p);
		left += l;
		right += l;
		top += t;
		bottom += t;
		p = YGNodeGetParent(p);
	}
	return js_yoga_make_layout(ctx, left, right, top, bottom,
							   YGNodeLayoutGetWidth(n),
							   YGNodeLayoutGetHeight(n));
}

static JSValue js_yoga_node_getter(JSContext *ctx, JSValueConst this_val, int magic)
{
	YGNodeRef n = JS_GetOpaque2(ctx, this_val, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	switch(magic) {
		case E_PositionType:
			return JS_NewInt32(ctx, YGNodeStyleGetPositionType(n));
		case E_AlignContent:
			return JS_NewInt32(ctx, YGNodeStyleGetAlignContent(n));
		case E_AlignItems:
			return JS_NewInt32(ctx, YGNodeStyleGetAlignItems(n));
		case E_AlignSelf:
			return JS_NewInt32(ctx, YGNodeStyleGetAlignSelf(n));
		case E_FlexDirection:
			return JS_NewInt32(ctx, YGNodeStyleGetFlexDirection(n));
		case E_FlexWrap:
			return JS_NewInt32(ctx, YGNodeStyleGetFlexWrap(n));
		case E_JustifyContent:
			return JS_NewInt32(ctx, YGNodeStyleGetJustifyContent(n));
//		case E_Margin: //TODO
//		case E_MarginPercent:
//		case E_MarginAuto:
		case E_Overflow:
			return JS_NewInt32(ctx, YGNodeStyleGetOverflow(n));
		case E_Display:
			return JS_NewInt32(ctx, YGNodeStyleGetDisplay(n));
		case E_Flex:
			return JS_NewFloat64(ctx, YGNodeStyleGetFlex(n));
		case E_FlexBasis:
			return js_wrap_YGValue(ctx, YGNodeStyleGetFlexBasis(n));
//		case E_FlexBasisPercent:
		case E_FlexGrow:
			return JS_NewFloat64(ctx, YGNodeStyleGetFlexGrow(n));
		case E_FlexShrink:
			return JS_NewFloat64(ctx, YGNodeStyleGetFlexShrink(n));
		case E_Width:
			return js_wrap_YGValue(ctx, YGNodeStyleGetWidth(n));
//		case E_WidthPercent:
//		case E_WidthAuto:
		case E_Height:
			return js_wrap_YGValue(ctx, YGNodeStyleGetHeight(n));
//		case E_HeightPercent:
//		case E_HeightAuto:
		case E_MinWidth:
			return js_wrap_YGValue(ctx, YGNodeStyleGetMinWidth(n));
//		case E_MinWidthPercent:
		case E_MinHeight:
			return js_wrap_YGValue(ctx, YGNodeStyleGetMinHeight(n));
//		case E_MinHeightPercent:
		case E_MaxWidth:
			return js_wrap_YGValue(ctx, YGNodeStyleGetMaxWidth(n));
//		case E_MaxWidthPercent:
		case E_MaxHeight:
			return js_wrap_YGValue(ctx, YGNodeStyleGetMaxHeight(n));
//		case E_MaxHeightPercent:
		case E_AspectRatio:
			return JS_NewFloat64(ctx, YGNodeStyleGetAspectRatio(n));
//		case E_Border: // TODO
//		case E_PaddingPercent:
//		case E_Parent:
//			return js_wrap_YGNode(ctx, YGNodeGetParent(n));
		case E_ChildCount:
			return JS_NewInt32(ctx, YGNodeGetChildCount(n));
		case E_Dirty:
			return JS_NewBool(ctx, YGNodeIsDirty(n));
		case E_ComputedLeft:
			return JS_NewFloat64(ctx, YGNodeLayoutGetLeft(n));
		case E_ComputedRight:
			return JS_NewFloat64(ctx, YGNodeLayoutGetRight(n));
		case E_ComputedTop:
			return JS_NewFloat64(ctx, YGNodeLayoutGetTop(n));
		case E_ComputedBottom:
			return JS_NewFloat64(ctx, YGNodeLayoutGetBottom(n));
		case E_ComputedWidth:
			return JS_NewFloat64(ctx, YGNodeLayoutGetWidth(n));
		case E_ComputedHeight:
			return JS_NewFloat64(ctx, YGNodeLayoutGetHeight(n));
//		case E_ComputedMargin: // TODO
//		case E_ComputedBorder: // TODO
//		case E_ComputedPadding: // TODO
			return JS_NewFloat64(ctx, YGNodeLayoutGetLeft(n));
		default:
			break;
	}
	return JS_EXCEPTION;
}

static JSValue js_yoga_node_setter_int(JSContext *ctx, JSValueConst this_val, JSValue val, int magic)
{
	YGNodeRef n = JS_GetOpaque2(ctx, this_val, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	int value = 0;
	if (JS_ToInt32(ctx, &value, val))
		return JS_EXCEPTION;
//	printf("setter_int: magic=%d value=%d\n", magic, value);
	switch(magic) {
		case E_PositionType:
			YGNodeStyleSetPositionType(n, (YGPositionType)value); break;
		case E_AlignContent:
			YGNodeStyleSetAlignContent(n, (YGAlign)value); break;
		case E_AlignSelf:
			YGNodeStyleSetAlignSelf(n, (YGAlign)value); break;
		case E_FlexDirection:
			YGNodeStyleSetFlexDirection(n, (YGFlexDirection)value); break;
		case E_FlexWrap:
			YGNodeStyleSetFlexWrap(n, (YGWrap)value); break;
		case E_JustifyContent:
			YGNodeStyleSetJustifyContent(n, (YGJustify)value); break;
		case E_AlignItems:
			YGNodeStyleSetAlignItems(n, (YGAlign)value); break;
		default:
			return JS_EXCEPTION;
	}
	return JS_UNDEFINED;
}

static JSValue js_yoga_node_setter_float(JSContext *ctx, JSValueConst this_val, JSValue val, int magic)
{
	YGNodeRef n = JS_GetOpaque2(ctx, this_val, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	float value = 0;
	if (JS_ToFloat32(ctx, &value, val))
		return JS_EXCEPTION;
//	printf("setter_float: magic=%d value=%f\n", magic, value);
	switch(magic) {
//		case E_PositionType:
//		case E_PositionPercent:
//		case E_AlignContent:
//		case E_AlignItems:
//		case E_AlignSelf:
//		case E_FlexDirection:
//		case E_FlexWrap:
//		case E_JustifyContent:
//		case E_Margin:
		case E_MarginPercent:
		case E_MarginAuto:
		case E_Overflow:
		case E_Display:
			break;
		case E_Flex:
			YGNodeStyleSetFlex(n, value); break;
		case E_FlexBasis:
		case E_FlexBasisPercent:
			break;
		case E_FlexGrow:
			YGNodeStyleSetFlexGrow(n, value); break;
		case E_FlexShrink:
			break;
		case E_Width:
			YGNodeStyleSetWidth(n, value); break;
		case E_WidthPercent:
			YGNodeStyleSetWidthPercent(n, value); break;
//		case E_WidthAuto:
		case E_Height:
			YGNodeStyleSetHeight(n, value); break;
		case E_HeightPercent:
			YGNodeStyleSetHeightPercent(n, value); break;
//		case E_HeightAuto:
		case E_MinWidth:
			YGNodeStyleSetMinWidth(n, value); break;
		case E_MinWidthPercent:
			YGNodeStyleSetMinWidthPercent(n, value); break;
		case E_MinHeight:
			YGNodeStyleSetMinHeight(n, value); break;
		case E_MinHeightPercent:
			YGNodeStyleSetMinHeightPercent(n, value); break;
		case E_MaxWidth:
			YGNodeStyleSetMaxWidth(n, value); break;
		case E_MaxWidthPercent:
			YGNodeStyleSetMaxWidthPercent(n, value); break;
		case E_MaxHeight:
			YGNodeStyleSetMaxHeight(n, value); break;
		case E_MaxHeightPercent:
			YGNodeStyleSetMaxHeightPercent(n, value); break;
		case E_AspectRatio:
			YGNodeStyleSetAspectRatio(n, value); break;
		case E_Border:
		case E_PaddingPercent:
		default:
			break;
	}
	return JS_UNDEFINED;
}

#define FUNC(fn) \
static JSValue js_yoga_##fn(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv)

#define FUNC_MAGIC(fn) \
static JSValue js_yoga_##fn(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv, int magic)

FUNC_MAGIC(getter2)
{
	if (argc != 1)
		return JS_EXCEPTION;
	YGNodeRef n = JS_GetOpaque2(ctx, this_value, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	int edge = 0;
	if (JS_ToInt32(ctx, &edge, argv[0]))
		return JS_EXCEPTION;
	if (magic == E_Position)
		return js_wrap_YGValue(ctx, YGNodeStyleGetPosition(n, (YGEdge)edge));
	else if (magic == E_Padding)
		return js_wrap_YGValue(ctx, YGNodeStyleGetPadding(n, (YGEdge)edge));
	else if (magic == E_Margin)
		return js_wrap_YGValue(ctx, YGNodeStyleGetMargin(n, (YGEdge)edge));
	return JS_EXCEPTION;
}

FUNC_MAGIC(setter2)
{
	if (argc != 2)
		return JS_EXCEPTION;
	YGNodeRef n = JS_GetOpaque2(ctx, this_value, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	int edge = 0;
	if (JS_ToInt32(ctx, &edge, argv[0]))
		return JS_EXCEPTION;
	float value;
	if (JS_ToFloat32(ctx, &value, argv[1]))
		return JS_EXCEPTION;
//	printf("%s:%d, %f\n", __FUNCTION__, edge, value);
	if (magic == E_Position)
		YGNodeStyleSetPosition(n, (YGEdge)edge, value);
	else if (magic == E_PositionPercent)
		YGNodeStyleSetPositionPercent(n, (YGEdge)edge, value);
	else if (magic == E_Padding)
		YGNodeStyleSetPadding(n, (YGEdge)edge, value);
	else if (magic == E_Margin)
		YGNodeStyleSetMargin(n, (YGEdge)edge, value);
	else
		return JS_EXCEPTION;
	return JS_UNDEFINED;
}

FUNC(calculateLayout)
{
	if (argc != 3)
		return JS_EXCEPTION;
	YGNodeRef n = JS_GetOpaque2(ctx, this_value, js_yoga_node_class_id);
	if (!n)
		return JS_EXCEPTION;
	float w = 0, h = 0;
	int dir = 0;
	JS_ToFloat32(ctx, &w, argv[0]);
	JS_ToFloat32(ctx, &h, argv[1]);
	JS_ToInt32(ctx, &dir, argv[2]);
	YGNodeCalculateLayout(n, w, h, (YGDirection)dir);
	return JS_UNDEFINED;
}

//FUNC(getChild)
//{
//	if (argc != 1)
//		return JS_EXCEPTION;
//	YGNodeRef n = JS_GetOpaque2(ctx, this_value, js_yoga_node_class_id);
//	if (!n)
//		return JS_EXCEPTION;
//	uint32_t index = 0;
//	if (JS_ToUint32(ctx, &index, argv[0]))
//		return JS_EXCEPTION;
//	return js_wrap_YGNode(ctx, YGNodeGetChild(n, index));
//}

FUNC(insertChild)
{
	YGNodeRef node = NULL, child = NULL;
	uint32_t index = 0;
	if (argc != 2)
		return JS_EXCEPTION;
	node = JS_GetOpaque2(ctx, this_value, js_yoga_node_class_id);
	if (!node)
		return JS_EXCEPTION;
	child = JS_GetOpaque2(ctx, argv[0], js_yoga_node_class_id);
	if (!child)
		return JS_EXCEPTION;
	if (JS_ToUint32(ctx, &index, argv[1]))
		return JS_EXCEPTION;
	YGNodeInsertChild(node, child, index);
	return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_yoga_node_proto_funcs[] = {
	JS_CGETSET_MAGIC_DEF("positionType", js_yoga_node_getter, js_yoga_node_setter_int, E_PositionType),
	JS_CFUNC_MAGIC_DEF("getPosition", 1, js_yoga_getter2, E_Position),
	JS_CFUNC_MAGIC_DEF("setPosition", 2, js_yoga_setter2, E_Position),
	JS_CFUNC_MAGIC_DEF("setPositionPercent", 2, js_yoga_setter2, E_PositionPercent),
	JS_CGETSET_MAGIC_DEF("alignContent", js_yoga_node_getter, js_yoga_node_setter_int, E_AlignContent),
	JS_CGETSET_MAGIC_DEF("alignItems", js_yoga_node_getter, js_yoga_node_setter_int, E_AlignItems),
	JS_CGETSET_MAGIC_DEF("alignSelf", js_yoga_node_getter, js_yoga_node_setter_int, E_AlignSelf),
	JS_CGETSET_MAGIC_DEF("flexDirection", js_yoga_node_getter, js_yoga_node_setter_int, E_FlexDirection),
	JS_CGETSET_MAGIC_DEF("flexWrap", js_yoga_node_getter, js_yoga_node_setter_int, E_FlexWrap),
	JS_CGETSET_MAGIC_DEF("justifyContent", js_yoga_node_getter, js_yoga_node_setter_int, E_JustifyContent),
	JS_CFUNC_MAGIC_DEF("getMargin", 1, js_yoga_getter2, E_Margin),
	JS_CFUNC_MAGIC_DEF("setMargin", 2, js_yoga_setter2, E_Margin),
	JS_CGETSET_MAGIC_DEF("flex", js_yoga_node_getter, js_yoga_node_setter_float, E_Flex),
	JS_CGETSET_MAGIC_DEF("flexGrow", js_yoga_node_getter, js_yoga_node_setter_float, E_FlexGrow),
//	JS_CFUNC_DEF("getChild", 1, js_yoga_getChild),
//	JS_CGETSET_MAGIC_DEF("parent", js_yoga_node_getter, NULL, E_Parent),
	JS_CGETSET_MAGIC_DEF("childCount", js_yoga_node_getter, NULL, E_ChildCount),
	JS_CGETSET_MAGIC_DEF("width", js_yoga_node_getter, js_yoga_node_setter_float, E_Width),
	JS_CGETSET_MAGIC_DEF("widthPercent", NULL, js_yoga_node_setter_float, E_WidthPercent),
	JS_CGETSET_MAGIC_DEF("height", js_yoga_node_getter, js_yoga_node_setter_float, E_Height),
	JS_CGETSET_MAGIC_DEF("heightPercent", NULL, js_yoga_node_setter_float, E_HeightPercent),
	JS_CGETSET_MAGIC_DEF("minWidth", js_yoga_node_getter, js_yoga_node_setter_float, E_MinWidth),
	JS_CGETSET_MAGIC_DEF("minHeight", js_yoga_node_getter, js_yoga_node_setter_float, E_MinHeight),
	JS_CGETSET_MAGIC_DEF("maxWidth", js_yoga_node_getter, js_yoga_node_setter_float, E_MaxWidth),
	JS_CGETSET_MAGIC_DEF("maxHeight", js_yoga_node_getter, js_yoga_node_setter_float, E_MaxHeight),
	JS_CGETSET_MAGIC_DEF("aspectRatio", js_yoga_node_getter, js_yoga_node_setter_float, E_AspectRatio),
	JS_CGETSET_MAGIC_DEF("computedLeft", js_yoga_node_getter, NULL, E_ComputedLeft),
	JS_CGETSET_MAGIC_DEF("computedRight", js_yoga_node_getter, NULL, E_ComputedRight),
	JS_CGETSET_MAGIC_DEF("computedTop", js_yoga_node_getter, NULL, E_ComputedTop),
	JS_CGETSET_MAGIC_DEF("computedBottom", js_yoga_node_getter, NULL, E_ComputedBottom),
	JS_CGETSET_MAGIC_DEF("computedWidth", js_yoga_node_getter, NULL, E_ComputedWidth),
	JS_CGETSET_MAGIC_DEF("computedHeight", js_yoga_node_getter, NULL, E_ComputedHeight),
	JS_CGETSET_DEF("computedLayout", js_yoga_node_getComputedLayout, NULL),
	JS_CGETSET_DEF("computedAbsoluteLayout", js_yoga_node_getComputedAbsoluteLayout, NULL),
	JS_CFUNC_MAGIC_DEF("getPadding", 1, js_yoga_getter2, E_Padding),
	JS_CFUNC_MAGIC_DEF("setPadding", 2, js_yoga_setter2, E_Padding),
	JS_CFUNC_DEF("calculateLayout", 3, js_yoga_calculateLayout),
	JS_CFUNC_DEF("insertChild", 2, js_yoga_insertChild),
};


#define YOGA_FLAG(name) JS_PROP_INT32_DEF(#name, YG##name, JS_PROP_CONFIGURABLE)

static const JSCFunctionListEntry js_yoga_funcs[] = {
	YOGA_FLAG(AlignAuto),
	YOGA_FLAG(AlignFlexStart),
	YOGA_FLAG(AlignCenter),
	YOGA_FLAG(AlignFlexEnd),
	YOGA_FLAG(AlignStretch),
	YOGA_FLAG(AlignBaseline),
	YOGA_FLAG(AlignSpaceBetween),
	YOGA_FLAG(AlignSpaceAround),
	
	YOGA_FLAG(DirectionInherit),
	YOGA_FLAG(DirectionLTR),
	YOGA_FLAG(DirectionRTL),
	
	YOGA_FLAG(EdgeLeft),
	YOGA_FLAG(EdgeTop),
	YOGA_FLAG(EdgeRight),
	YOGA_FLAG(EdgeBottom),
	YOGA_FLAG(EdgeStart),
	YOGA_FLAG(EdgeEnd),
	YOGA_FLAG(EdgeHorizontal),
	YOGA_FLAG(EdgeVertical),
	YOGA_FLAG(EdgeAll),
	
	YOGA_FLAG(FlexDirectionColumn),
	YOGA_FLAG(FlexDirectionColumnReverse),
	YOGA_FLAG(FlexDirectionRow),
	YOGA_FLAG(FlexDirectionRowReverse),
	
	YOGA_FLAG(JustifyFlexStart),
	YOGA_FLAG(JustifyCenter),
	YOGA_FLAG(JustifyFlexEnd),
	YOGA_FLAG(JustifySpaceBetween),
	YOGA_FLAG(JustifySpaceAround),
	YOGA_FLAG(JustifySpaceEvenly),
	
	YOGA_FLAG(LogLevelError),
	YOGA_FLAG(LogLevelWarn),
	YOGA_FLAG(LogLevelInfo),
	YOGA_FLAG(LogLevelDebug),
	YOGA_FLAG(LogLevelVerbose),
	YOGA_FLAG(LogLevelFatal),
	
	YOGA_FLAG(MeasureModeUndefined),
	YOGA_FLAG(MeasureModeExactly),
	YOGA_FLAG(MeasureModeAtMost),
	
	YOGA_FLAG(NodeTypeDefault),
	YOGA_FLAG(NodeTypeText),
	
	YOGA_FLAG(OverflowVisible),
	YOGA_FLAG(OverflowHidden),
	YOGA_FLAG(OverflowScroll),
	
	YOGA_FLAG(PositionTypeRelative),
	YOGA_FLAG(PositionTypeAbsolute),
	
	YOGA_FLAG(PrintOptionsLayout),
	YOGA_FLAG(PrintOptionsStyle),
	YOGA_FLAG(PrintOptionsChildren),
	
	YOGA_FLAG(UnitUndefined),
	YOGA_FLAG(UnitPoint),
	YOGA_FLAG(UnitPercent),
	YOGA_FLAG(UnitAuto),
	
	YOGA_FLAG(WrapNoWrap),
	YOGA_FLAG(WrapWrap),
	YOGA_FLAG(WrapWrapReverse),
};


static int js_yoga_init(JSContext *ctx, JSModuleDef *m)
{
	JSValue node_proto, node_class;
	
	JS_NewClassID(&js_yoga_node_class_id);
	JS_NewClass(JS_GetRuntime(ctx), js_yoga_node_class_id, &js_yoga_node_class);
	
	node_proto = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, node_proto, js_yoga_node_proto_funcs, countof(js_yoga_node_proto_funcs));
	JS_SetClassProto(ctx, js_yoga_node_class_id, node_proto);
	
	node_class = JS_NewCFunction2(ctx, js_yoga_node_ctor, "node", 0, JS_CFUNC_constructor, 0);
	JS_SetConstructor(ctx, node_class, node_proto);
	
	JS_SetModuleExport(ctx, m, "node", node_class);
	JS_SetModuleExportList(ctx, m, js_yoga_funcs, countof(js_yoga_funcs));
	return 0;
}

JSModuleDef *js_init_module_yoga(JSContext *ctx, const char *module_name)
{
	JSModuleDef *m;
	m = JS_NewCModule(ctx, module_name, js_yoga_init);
	if (!m)
		return NULL;
	JS_AddModuleExport(ctx, m, "node");
	JS_AddModuleExportList(ctx, m, js_yoga_funcs, countof(js_yoga_funcs));
	return m;
}
