## 这个是zygisk-il2cppdumper的源代码， 我需要扩展一下功能：
- 在dump完il2cpp以后， 我需要调用 il2cpp原有的逻辑函数。 
## 我先有有一个EquipConfig序列化以后的文件， 我放到/sdcard/bwxrk/config/raw/EquipConfig
- EquipConfig文件结构说明： 文件头(ResFileHead) + 序列化对象数组
- 通用的文件头 internal class ResFileHead
{
	// Fields
	public UInt32 tag; // 0x10
	public UInt32 len; // 0x14
	public UInt32 version; // 0x18
	public UInt32 resnum; // 0x1c
	public UInt32 crc32; // 0x20

	// Properties

	// Methods
	// RVA: 0x1127138 VA: 0x74c9027138
	public Void .ctor() { }
}

## 磁盘中文件头中重要信息： 
- tag: 固定： 00 00 2D EF  
- len: 整个文件的大小
- version: 可以忽略
- resnum: 数据条数
- crc32: crc 可以忽略不管

- 解析出文件头以后， 利用 EquipConfig的unpack逐个还原对象
// Dll : GameLogic.dll
// Namespace: C6Game
internal class EquipConfigData : ItemConfigData
{
	// Fields
	private EquipConfig m_equipCfg; // 0x18

	// Properties
	public override EquipConfig EquipCfg { get; }
	public override UInt32 ItemID { get; }
	public override Byte Quality { get; }
	public override String ItemName { get; }
	public override String IconName { get; }
	public override String Desc { get; }
	public override Int32 WearMinLv { get; }

	// Methods
	// RVA: 0x151f848 VA: 0x74c941f848
	public Void .ctor(EquipConfig equipCfg) { }
	// RVA: 0x151f8c4 VA: 0x74c941f8c4
	public override EquipConfig get_EquipCfg() { }
	// RVA: 0x151f8cc VA: 0x74c941f8cc
	public override UInt32 get_ItemID() { }
	// RVA: 0x151f908 VA: 0x74c941f908
	public override Byte get_Quality() { }
	// RVA: 0x151f910 VA: 0x74c941f910
	public override String get_ItemName() { }
	// RVA: 0x151f990 VA: 0x74c941f990
	public override String get_IconName() { }
	// RVA: 0x151fa94 VA: 0x74c941fa94
	public override String get_Desc() { }
	// RVA: 0x151fb48 VA: 0x74c941fb48
	public override Int32 get_WearMinLv() { }
}

// Dll : DodProtoBase.dll
// Namespace: ProtoBase
public class PbReadBuf
{
	// Fields
	private Byte[] beginPtr; // 0x10
	private Int32 position; // 0x18
	private Int32 length; // 0x1c
	private Boolean IsNetEndian; // 0x20

	// Properties

	// Methods
	// RVA: 0x1177610 VA: 0x74c9077610
	public Void .ctor() { }
	// RVA: 0x1177634 VA: 0x74c9077634
	public Void .ctor(Byte[] ptr, Int32 len) { }
	// RVA: 0x1177670 VA: 0x74c9077670
	public Void set(Byte[] ptr, Int32 len) { }
	// RVA: 0x1177688 VA: 0x74c9077688
	public Int32 getUsedSize() { }
	// RVA: 0x1177690 VA: 0x74c9077690
	public Void setUsedSize(Int32 pos) { }
	// RVA: 0x11776a8 VA: 0x74c90776a8
	public Int32 getTotalSize() { }
	// RVA: 0x11776b0 VA: 0x74c90776b0
	public Int32 getLeftSize() { }
	// RVA: 0x11776bc VA: 0x74c90776bc
	public ErrorType readInt8(out SByte dest) { }
	// RVA: 0x11776e8 VA: 0x74c90776e8
	public ErrorType readUInt8(out Byte dest) { }
	// RVA: 0x1177748 VA: 0x74c9077748
	public ErrorType readInt16(out Int16 dest) { }
	// RVA: 0x117659c VA: 0x74c907659c
	public ErrorType readUInt16(out UInt16 dest) { }
	// RVA: 0x1177864 VA: 0x74c9077864
	public ErrorType readInt32(out Int32 dest) { }
	// RVA: 0x11765c8 VA: 0x74c90765c8
	public ErrorType readUInt32(out UInt32 dest) { }
	// RVA: 0x1177980 VA: 0x74c9077980
	public ErrorType readInt64(out Int64 dest) { }
	// RVA: 0x1177a9c VA: 0x74c9077a9c
	public ErrorType readUInt64(out UInt64 dest) { }
	// RVA: 0x1177ac8 VA: 0x74c9077ac8
	private Void ConvertBytesEndian(Byte[] ptr, Int32 pos) { }
	// RVA: 0x1177b44 VA: 0x74c9077b44
	public ErrorType readFloat(out Single dest) { }
	// RVA: 0x1177c64 VA: 0x74c9077c64
	public ErrorType readString(out String dest, Int32 maxStrSize) { }
	// RVA: 0x1177d54 VA: 0x74c9077d54
	public ErrorType readInt16(out Int16 dest, Int32 pos) { }
	// RVA: 0x1177e68 VA: 0x74c9077e68
	public ErrorType readUInt16(out UInt16 dest, Int32 pos) { }
	// RVA: 0x1177e94 VA: 0x74c9077e94
	public ErrorType readInt32(out Int32 dest, Int32 pos) { }
	// RVA: 0x1177fa8 VA: 0x74c9077fa8
	public ErrorType readUInt32(out UInt32 dest, Int32 pos) { }
}

- 最后得到EquipConfig对象数组, 把这个数组存为json文件: /sdcard/bwxrk/config/raw/EquipConfig.json
