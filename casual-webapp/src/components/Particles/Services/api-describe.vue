<template>
    <div>
        <div v-for="(attr, index) in yaml" :key="index">
            <div v-if="attr" class="api-describer api-describer__wrapper" :class="'api-describer__'+ attr.level">
                <div class="api-describer api-describer__highlight">{{attr.category}}</div>
                <div class="api-describer api-describer__role">{{attr.role}}</div>
            </div>
        </div>

    </div>
</template>

<script>
    export default {
        name: "API-describe",
        props: ['api'],
        data() {
            return {
                yaml: []
            }
        },
        created() {
            this.yaml = this.getYaml(this.api);
        },
        methods: {
            getYaml: function (json) {
                return this.toYaml(json);
            }
        }
    }
</script>

<style scoped>

</style>